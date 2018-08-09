
#define null 0

#define PAW_GETTER(TYPE, VAR_NAME) \
  inline TYPE VAR_NAME() const { return VAR_NAME##_; }

#define PAW_SETTER(TYPE, VAR_NAME) \
  inline void VAR_NAME(TYPE value) { VAR_NAME##_ = value; }

#define PAW_GETTER_SETTER(TYPE, VAR_NAME)  \
  PAW_GETTER(TYPE, VAR_NAME) \
  PAW_SETTER(TYPE, VAR_NAME)

