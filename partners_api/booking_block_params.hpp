#pragma once

#include "partners_api/booking_params_base.hpp"

#include "base/url_helpers.hpp"

#include <string>
#include <vector>

namespace booking
{
struct BlockParams : public ParamsBase
{
  using Extras = std::vector<std::string>;

  static BlockParams MakeDefault();

  base::url::Params Get() const;

  // ParamsBase overrides:
  bool IsEmpty() const override;
  bool Equals(ParamsBase const & rhs) const override;
  bool Equals(BlockParams const & lhs) const override;
  void Set(ParamsBase const & src) override;

  std::string m_hotelId;
  std::string m_currency;
  Time m_checkin;
  Time m_checkout;
  Extras m_extras;
  std::string m_language;
};
}  // namespce booking
