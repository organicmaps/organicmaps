#pragma once

#include "base/string_utils.hpp"

#include <string>
#include <type_traits>
#include <vector>

namespace base
{
// Simple id generator which increments received value and concatenates when
// BaseIntegralUnsigned max value is reached.
template <typename BaseIntegralUnsigned = uint64_t>
class IdGenerator
{
public:
  static_assert(std::is_integral<BaseIntegralUnsigned>::value, "Integral types is supported only");
  static_assert(std::is_unsigned<BaseIntegralUnsigned>::value, "Unsigned types is supported only");

  using Id = std::string;

  static Id GetInitialId()
  {
    return std::to_string(kMinIdInternal);
  }

  static Id GetNextId(Id const & id)
  {
    std::vector<Id> idParts;
    auto first = id.begin();
    auto last = id.end();
    while (last - first > kMaxSymbolsIdInternal)
    {
      idParts.emplace_back(first, first + kMaxSymbolsIdInternal);
      first += kMaxSymbolsIdInternal;
    }
    idParts.emplace_back(first, last);

    IdInternal internalId;
    if (!strings::to_any(idParts.back(), internalId))
      return {};

    auto const newId = MakeNextInternalId(internalId);

    if (newId < internalId)
      idParts.emplace_back(std::to_string(newId));
    else
      idParts.back() = std::to_string(newId);

    return strings::JoinAny(idParts, "");
  }

private:
  using IdInternal = BaseIntegralUnsigned;

  static IdInternal constexpr kIncorrectId = 0;
  static IdInternal constexpr kMinIdInternal = 1;
  static IdInternal constexpr kMaxIdInternal = std::numeric_limits<IdInternal>::max();
  static auto constexpr kMaxSymbolsIdInternal = std::numeric_limits<IdInternal>::digits10 + 1;

  static constexpr IdInternal MakeNextInternalId(IdInternal id)
  {
    if (id == kMaxIdInternal)
      return kMinIdInternal;

    return ++id;
  }
};
}  // namespace base
