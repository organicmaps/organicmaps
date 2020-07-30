#pragma once

#include "partners_api/booking_ordering_params.hpp"
#include "partners_api/booking_params_base.hpp"

#include "coding/url.hpp"

#include <string>
#include <vector>

namespace booking
{
struct BlockParams : public ParamsBase
{
  using Extras = std::vector<std::string>;

  static BlockParams MakeDefault();

  url::Params Get() const;

  // ParamsBase overrides:
  bool IsEmpty() const override;
  bool Equals(ParamsBase const & rhs) const override;
  bool Equals(BlockParams const & lhs) const override;
  void Set(ParamsBase const & src) override;

  std::string m_hotelId;
  OrderingParams m_orderingParams;
  std::string m_currency;
  Extras m_extras;
  std::string m_language;
};
}  // namespce booking
