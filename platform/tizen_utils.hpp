#pragma once

#include "std/target_os.hpp"

#include <string>

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
std::string FromTizenString(Tizen::Base::String const & str_tizen);
std::string CodeFromISO369_2to_1(std::string const & code);
std::string GetLanguageCode(Tizen::Locales::LanguageCode code);
std::string GetTizenLocale();

#endif
