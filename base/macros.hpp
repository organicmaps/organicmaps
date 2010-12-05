#pragma once

namespace my
{
    namespace impl
    {
        // http://rsdn.ru/Forum/?mid=1025325
        template <typename T, unsigned int N> char(&ArraySize(T(&)[N]))[N];
    }
}

// Number of elements in array. Compilation error if the type passed is not an array.
#define ARRAY_SIZE(X) sizeof(::my::impl::ArraySize(X))

// Make class noncopyable.
#define NONCOPYABLE(class_name)								\
private:													\
    class_name const & operator = (class_name const &);		\
    class_name(class_name const &);
/////////////////////////////////////////////////////////////

#define TO_STRING_IMPL(x) #x
#define TO_STRING(x) TO_STRING_IMPL(x)

#define UNUSED_VALUE(x) do { (void)(x); } while (false)

namespace my
{
  namespace impl
  {
    template <typename T> inline void ForceUseValue(T const & t)
    {
       volatile T dummy = t;
       UNUSED_VALUE(dummy);
    }
  }
}

// Prevent compiler optimization.
#define FORCE_USE_VALUE(x) ::my::impl::ForceUseValue(x)

#ifdef __GNUC__
#define PREDICT(x, prediction) __builtin_expect(x, prediction)
#define PREDICT_TRUE(x) __builtin_expect((x) != 0, 1)
#define PREDICT_FALSE(x) __builtin_expect((x) != 0, 0)
#else
#define PREDICT(x, prediction) (x)
#define PREDICT_TRUE(x) (x)
#define PREDICT_FALSE(x) (x)
#endif
