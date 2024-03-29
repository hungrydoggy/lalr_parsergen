#ifndef PAW_PRINT_SRC_DEFINES_H_
#define PAW_PRINT_SRC_DEFINES_H_

#include <stdint.h>


#define null 0
#define appetizer_null 0

#define PAW_GETTER(TYPE, VAR_NAME) \
inline TYPE VAR_NAME() const { return VAR_NAME##_; }

#define PAW_SETTER(TYPE, VAR_NAME) \
inline void VAR_NAME(TYPE value) { VAR_NAME##_ = value; }

#define PAW_GETTER_SETTER(TYPE, VAR_NAME)  \
PAW_GETTER(TYPE, VAR_NAME) \
PAW_SETTER(TYPE, VAR_NAME)


#ifdef PAW_PRINT_NO_EXPORTS
	#define PAW_PRINT_API 
#elif defined(_WINDOWS)
	#ifdef PAW_PRINT_EXPORTS
		#define PAW_PRINT_API __declspec(dllexport)
	#else
		#define PAW_PRINT_API __declspec(dllimport)
	#endif
#else
	#define PAW_PRINT_API 
#endif


namespace paw_print {

using byte   = unsigned char;
using ushort = unsigned short;
using uint   = unsigned int;
using int64  = int64_t;
using uint64 = uint64_t;

using DataType = byte;

}

#endif  // ifndef PAW_PRINT_SRC_DEFINES_H_
