#pragma once
#include "../../std/string.hpp"

namespace Tizen
{
  namespace Base
  {
    class String;
  }
}

//Convert from Tizen string to std::string
string FromTizenString(Tizen::Base::String const & str_tizen);
