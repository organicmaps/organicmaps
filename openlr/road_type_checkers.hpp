#pragma once

#include "indexer/ftypes_matcher.hpp"

namespace openlr
{
class TrunkChecker final : public ftypes::BaseChecker
{
public:
  TrunkChecker();
};

class PrimaryChecker final : public ftypes::BaseChecker
{
public:
  PrimaryChecker();
};

class SecondaryChecker final : public ftypes::BaseChecker
{
public:
  SecondaryChecker();
};

class TertiaryChecker final : public ftypes::BaseChecker
{
public:
  TertiaryChecker();
};

class ResidentialChecker final : public ftypes::BaseChecker
{
public:
  ResidentialChecker();
};

class LivingStreetChecker final : public ftypes::BaseChecker
{
public:
  LivingStreetChecker();
};
}  // namespace openlr
