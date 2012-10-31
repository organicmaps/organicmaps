#include "../../testing/testing.hpp"

#include "../regexp.hpp"


UNIT_TEST(RegExp_Or)
{
  regexp::RegExpT exp;
  regexp::Create("\\.mwm\\.(downloading2?$|resume2?$)", exp);

  TEST(regexp::IsExist("Aruba.mwm.downloading", exp), ());
  TEST(!regexp::IsExist("Aruba.mwm.downloading1", exp), ());
  TEST(regexp::IsExist("Aruba.mwm.downloading2", exp), ());
  TEST(!regexp::IsExist("Aruba.mwm.downloading3", exp), ());
  TEST(!regexp::IsExist("Aruba.mwm.downloading.tmp", exp), ());

  TEST(regexp::IsExist("Aruba.mwm.resume", exp), ());
  TEST(!regexp::IsExist("Aruba.mwm.resume1", exp), ());
  TEST(regexp::IsExist("Aruba.mwm.resume2", exp), ());
  TEST(!regexp::IsExist("Aruba.mwm.resume3", exp), ());
  TEST(!regexp::IsExist("Aruba.mwm.resume.tmp", exp), ());
}
