#pragma once

#include "generator/sponsored_dataset.hpp"
#include "generator/sponsored_object_base.hpp"

#include "geometry/latlon.hpp"

#include "base/newtype.hpp"

#include <limits>
#include <string>

namespace generator
{
struct OpentableRestaurant : SponsoredObjectBase
{
  enum class Fields
  {
    Id = 0,
    Latitude,
    Longtitude,
    Name,
    Address,
    DescUrl,
    Phone,
    // Opentable doesn't have translations.
    // Translations,
    Counter
  };

  explicit OpentableRestaurant(std::string const & src);

  static constexpr size_t FieldIndex(Fields field) { return SponsoredObjectBase::FieldIndex(field); }
  static constexpr size_t FieldsCount() { return SponsoredObjectBase::FieldsCount<Fields>(); }

  // string m_translations;
};

using OpentableDataset = SponsoredDataset<OpentableRestaurant>;
}  // namespace generator
