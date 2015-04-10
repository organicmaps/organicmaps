#pragma once

#include "std/stdint.hpp"

struct HttpFinishedParams;

namespace appstore
{
  class Client
  {
    /// @name HTTP callbacks
    //@{
    void OnVersionCheck(HttpFinishedParams const & params);
    //@}

  public:
    void RequestVersionCheck(int64_t countriesVersion, int64_t purchasesVersion);
  };
}
