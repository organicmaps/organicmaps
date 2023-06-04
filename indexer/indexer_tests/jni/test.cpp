#include "./test.hpp"

#include "android/UnitTests/jni/mock.hpp"


extern int main(int argc, char * argv[]);

namespace indexer_tests
{
  void test(android_app * state)
  {
    Initialize(state);
    main(0, nullptr);
  }
}
