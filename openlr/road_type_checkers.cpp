#include "openlr/road_type_checkers.hpp"

#include "indexer/classificator.hpp"

namespace openlr
{
// TrunkChecker ------------------------------------------------------------------------------------
TrunkChecker::TrunkChecker()
{
  auto const & c = classif();
  m_types.push_back(c.GetTypeByPath({"highway", "motorway"}));
  m_types.push_back(c.GetTypeByPath({"highway", "motorway_link"}));
  m_types.push_back(c.GetTypeByPath({"highway", "trunk"}));
  m_types.push_back(c.GetTypeByPath({"highway", "trunk_link"}));
}

// PrimaryChecker ----------------------------------------------------------------------------------
PrimaryChecker::PrimaryChecker()
{
  auto const & c = classif();
  m_types.push_back(c.GetTypeByPath({"highway", "primary"}));
  m_types.push_back(c.GetTypeByPath({"highway", "primary_link"}));
}

// SecondaryChecker --------------------------------------------------------------------------------
SecondaryChecker::SecondaryChecker()
{
  auto const & c = classif();
  m_types.push_back(c.GetTypeByPath({"highway", "secondary"}));
  m_types.push_back(c.GetTypeByPath({"highway", "secondary_link"}));
}

// TertiaryChecker ---------------------------------------------------------------------------------
TertiaryChecker::TertiaryChecker()
{
  auto const & c = classif();
  m_types.push_back(c.GetTypeByPath({"highway", "tertiary"}));
  m_types.push_back(c.GetTypeByPath({"highway", "tertiary_link"}));
}

// ResidentialChecker ------------------------------------------------------------------------------
ResidentialChecker::ResidentialChecker()
{
  auto const & c = classif();
  m_types.push_back(c.GetTypeByPath({"highway", "road"}));
  m_types.push_back(c.GetTypeByPath({"highway", "unclassified"}));
  m_types.push_back(c.GetTypeByPath({"highway", "residential"}));
}

// LivingStreetChecker -----------------------------------------------------------------------------
LivingStreetChecker::LivingStreetChecker()
{
  auto const & c = classif();
  m_types.push_back(c.GetTypeByPath({"highway", "living_street"}));
}
}  // namespace openlr
