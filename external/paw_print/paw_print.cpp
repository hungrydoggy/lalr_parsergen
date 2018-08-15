#include "./paw_print.h"

#include <algorithm>
#include <iostream>


namespace paw_print {

    

PawPrint::PawPrint ()
:is_closed_(false) {
}

PawPrint::PawPrint (const vector<unsigned char> &raw_data)
:PawPrint() {
    setRawData(raw_data);
}

PawPrint::PawPrint (const Cursor &cursor)
:PawPrint() {
    operator = (cursor);
}

const PawPrint& PawPrint::operator = (const Cursor &cursor) {
    auto cursor_idx = cursor.idx();
    auto data_size = cursor.paw_print()->dataSize(cursor_idx);

    raw_data_.resize(data_size);
    column_map_.clear();
    line_map_  .clear();

    // copy raw_data
    auto &cursor_raw_data = cursor.paw_print()->raw_data_;
    for (int di=0; di<raw_data_.size(); ++di) {
        raw_data_[di] = cursor_raw_data[di + cursor_idx];
    }

    // copy column_map
    auto &cursor_column_map = cursor.paw_print()->column_map_;
    for (auto &itr : cursor_column_map) {
        auto idx = itr.first - cursor_idx;
        if (idx < 0 || idx >= data_size)
            continue;

        column_map_[idx] = itr.second;
    }

    // copy line_map
    auto &cursor_line_map = cursor.paw_print()->line_map_;
    for (auto &itr : cursor_line_map) {
        auto idx = itr.first - cursor_idx;
        if (idx < 0 || idx >= data_size)
            continue;

        line_map_[idx] = itr.second;
    }

    return *this;
}

PawPrint::~PawPrint () {
}

PawPrint::Cursor PawPrint::root () const {
    if (raw_data_.size() <= 0)
        return Cursor(this, -1);

    return Cursor(this, 0);
}

DataType PawPrint::type (int idx) const {
    return getData<DataType>(idx);
}

PawPrint::Data::StrSizeType PawPrint::getStrSize (int idx) const {
    return getData<PawPrint::Data::StrSizeType>(idx + sizeof(DataType));
}

const char* PawPrint::getStrValue (int idx) const {
    return (const char*)&raw_data_[idx + sizeof(DataType) + sizeof(PawPrint::Data::StrSizeType)];
}

int PawPrint::getKeyRawIdxOfPair (int pair_idx) const {
    return pair_idx + sizeof(DataType);
}

int PawPrint::getValueRawIdxOfPair (int pair_idx) const {
    auto key_idx = getKeyRawIdxOfPair(pair_idx);
    return key_idx + dataSize(key_idx);
}

int PawPrint::dataSize (int idx) const {
    int result = sizeof(DataType);

    auto t = type(idx);
    switch (t) {
        case Data::TYPE_INT   : result += sizeof(int   ); break;
        case Data::TYPE_DOUBLE: result += sizeof(double); break;

        case Data::TYPE_STRING:
            result += sizeof(Data::StrSizeType) + sizeof(const char) * getStrSize(idx);
            break;

        case Data::TYPE_SEQUENCE_START:
			for (int raw_idx : getDataIdxsOfSequence(idx))
				result += dataSize(raw_idx);
			result += sizeof(DataType);
			break;

        case Data::TYPE_MAP_START:
			for (int raw_idx : getDataIdxsOfMap(idx))
				result += dataSize(raw_idx);
			result += sizeof(DataType);
			break;

        case Data::TYPE_KEY_VALUE_PAIR:
            result += dataSize(idx + result); // key size
            result += dataSize(idx + result); // value size
            break;

        default: break;
    }

    return result;
}

const vector<int>& PawPrint::getDataIdxsOfSequence (int sequence_idx) const {
    if (data_idxs_of_sequence_map_.find(sequence_idx) != data_idxs_of_sequence_map_.end())
        return data_idxs_of_sequence_map_[sequence_idx];

    auto &result = data_idxs_of_sequence_map_[sequence_idx];
    auto idx = sequence_idx + sizeof(DataType);
    while (type(idx) != Data::TYPE_SEQUENCE_END) {
        result.push_back(idx);

        idx += dataSize(idx);
    }
    
    return result;
}

class SortFuncForKey {
public:
    SortFuncForKey (const PawPrint &paw_print)
    :paw_print_(paw_print) {
    }

    bool operator () (int a_idx, int b_idx) {
		auto a_key_idx = paw_print_.getKeyRawIdxOfPair(a_idx);
		auto b_key_idx = paw_print_.getKeyRawIdxOfPair(b_idx);
        return strcmp(paw_print_.getStrValue(a_key_idx), paw_print_.getStrValue(b_key_idx)) < 0;
    }

private:
    const PawPrint &paw_print_;
};

const vector<int>& PawPrint::getDataIdxsOfMap (int map_idx) const {
    if (data_idxs_of_map_map_.find(map_idx) != data_idxs_of_map_map_.end())
        return data_idxs_of_map_map_[map_idx];

    // make datas
    auto &result = data_idxs_of_map_map_[map_idx];
    auto idx = map_idx + sizeof(DataType);
    while (type(idx) != Data::TYPE_MAP_END) {
        result.push_back(idx);

        idx += dataSize(idx);
    }

    std::sort(result.begin(), result.end(), SortFuncForKey(*this));
    
    return result;
}

int PawPrint::findRawIdxOfValue (
        const vector<int> &map_datas,
        int first,
        int last,
        const char *key) const {

    if (first > last)
        return -1;

    int mid = (first + last) / 2;
    auto mid_pair_idx = map_datas[mid];
	auto mid_key_idx = getKeyRawIdxOfPair(mid_pair_idx);
    auto mid_key = getStrValue(mid_key_idx);

    auto cmp_res = strcmp(key, mid_key);
    if (cmp_res < 0)
        return findRawIdxOfValue(map_datas, first, mid - 1, key);
    else if (cmp_res > 0)
        return findRawIdxOfValue(map_datas, mid + 1, last, key);
    else
        return mid_pair_idx;
}

void PawPrint::pushBool (bool value, int column, int line) {
    if (is_closed_ == true)
        return;

    int old_size = raw_data_.size();
    raw_data_.resize(old_size + sizeof(DataType) + sizeof(char));
    *((DataType*)&raw_data_[old_size                   ]) = Data::TYPE_BOOL;
    *((char*     )&raw_data_[old_size + sizeof(DataType)]) = (char)value;

    if (column >= 0)
        column_map_[old_size] = (unsigned short)column;
    if (line >= 0)
        line_map_[old_size] = (unsigned short)line;
}

void PawPrint::pushInt (int value, int column, int line) {
    if (is_closed_ == true)
        return;

    int old_size = raw_data_.size();
    raw_data_.resize(old_size + sizeof(DataType) + sizeof(int));
    *((DataType*)&raw_data_[old_size                   ]) = Data::TYPE_INT;
    *((int*     )&raw_data_[old_size + sizeof(DataType)]) = value;

    if (column >= 0)
        column_map_[old_size] = (unsigned short)column;
    if (line >= 0)
        line_map_[old_size] = (unsigned short)line;
}

void PawPrint::pushDouble (double value, int column, int line) {
    if (is_closed_ == true)
        return;

    int old_size = raw_data_.size();
    raw_data_.resize(old_size + sizeof(DataType) + sizeof(double));
    *((DataType*)&raw_data_[old_size                   ]) = Data::TYPE_DOUBLE;
    *((double*  )&raw_data_[old_size + sizeof(DataType)]) = value;

    if (column >= 0)
        column_map_[old_size] = (unsigned short)column;
    if (line >= 0)
        line_map_[old_size] = (unsigned short)line;
}

void PawPrint::pushString (const char *value, int column, int line) {
    if (is_closed_ == true)
        return;

    int old_size = raw_data_.size();
    int str_count = strlen(value) + 1;
    raw_data_.resize(
            old_size
            + sizeof(DataType)
            + sizeof(Data::StrSizeType)
            + sizeof(const char) * str_count);
    *((DataType*         )&raw_data_[old_size                   ]) = Data::TYPE_STRING;
    *((Data::StrSizeType*)&raw_data_[old_size + sizeof(DataType)]) = str_count;
    auto p = (const char*)&raw_data_[
            old_size + sizeof(DataType) + sizeof(Data::StrSizeType)];
    memcpy((void*)p, (void*)value, sizeof(const char) * str_count);

    if (column >= 0)
        column_map_[old_size] = (unsigned short)column;
    if (line >= 0)
        line_map_[old_size] = (unsigned short)line;
}

void PawPrint::beginSequence (int column, int line) {
    if (is_closed_ == true)
        return;

    int old_size = raw_data_.size();
    raw_data_.resize(old_size + sizeof(DataType));
    *((DataType*)&raw_data_[old_size]) = Data::TYPE_SEQUENCE_START;

    if (column >= 0)
        column_map_[old_size] = (unsigned short)column;
    if (line >= 0)
        line_map_[old_size] = (unsigned short)line;
}

void PawPrint::endSequence (int column, int line) {
    if (is_closed_ == true)
        return;

    int old_size = raw_data_.size();
    raw_data_.resize(old_size + sizeof(DataType));
    *((DataType*)&raw_data_[old_size]) = Data::TYPE_SEQUENCE_END;

    if (column >= 0)
        column_map_[old_size] = (unsigned short)column;
    if (line >= 0)
        line_map_[old_size] = (unsigned short)line;
}

void PawPrint::pushKeyValuePair (int column, int line) {
    if (is_closed_ == true)
        return;

    int old_size = raw_data_.size();
    raw_data_.resize(old_size + sizeof(DataType));
    *((DataType*)&raw_data_[old_size]) = Data::TYPE_KEY_VALUE_PAIR;

    if (column >= 0)
        column_map_[old_size] = (unsigned short)column;
    if (line >= 0)
        line_map_[old_size] = (unsigned short)line;
}

void PawPrint::pushKey (const char *value, int column, int line) {
    if (is_closed_ == true)
        return;

    pushKeyValuePair(column, line);

    pushString(value, column, line);
}

void PawPrint::beginMap (int column, int line) {
    if (is_closed_ == true)
        return;

    int old_size = raw_data_.size();
    raw_data_.resize(old_size + sizeof(DataType));
    *((DataType*)&raw_data_[old_size]) = Data::TYPE_MAP_START;

    if (column >= 0)
        column_map_[old_size] = (unsigned short)column;
    if (line >= 0)
        line_map_[old_size] = (unsigned short)line;
}

void PawPrint::endMap (int column, int line) {
    if (is_closed_ == true)
        return;

    int old_size = raw_data_.size();
    raw_data_.resize(old_size + sizeof(DataType));
    *((DataType*)&raw_data_[old_size]) = Data::TYPE_MAP_END;

    if (column >= 0)
        column_map_[old_size] = (unsigned short)column;
    if (line >= 0)
        line_map_[old_size] = (unsigned short)line;
}

void PawPrint::setRawData (const vector<unsigned char> &raw_data) {
    raw_data_ = raw_data;
}

int PawPrint::getColumn (int idx) const {
    if (column_map_.find(idx) == column_map_.end())
        return -1;

    return column_map_.at(idx);
}

int PawPrint::getLine (int idx) const {
    if (line_map_.find(idx) == line_map_.end())
        return -1;

    return line_map_.at(idx);
}

}
