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

#endif
