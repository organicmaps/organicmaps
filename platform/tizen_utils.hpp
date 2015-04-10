#pragma once
#include "std/string.hpp"
#include "std/target_os.hpp"
#include <FLocales.h>

#ifdef OMIM_OS_TIZEN
namespace Tizen
{
  namespace Base
  {
    class String;
  }
}

//Convert from Tizen string to std::string
string FromTizenString(Tizen::Base::String const & str_tizen);
string CodeFromISO369_2to_1(string const & code);
string GetLanguageCode(Tizen::Locales::LanguageCode code);
string GetTizenLocale();

#endif
