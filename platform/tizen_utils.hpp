#pragma once
#include "../std/string.hpp"
#include "../std/target_os.hpp"

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
string CodeFromISO369_2to_1(string const & code_ISO_639_2);
string GetTizenLocale();

#endif
