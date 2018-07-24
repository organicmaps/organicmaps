#include "indexer/road_shields_parser.hpp"

#include "indexer/feature.hpp"

#include "base/string_utils.hpp"

#include <algorithm>
#include <array>
#include <unordered_map>
#include <utility>

namespace
{
using ftypes::RoadShield;
using ftypes::RoadShieldType;

uint32_t constexpr kMaxRoadShieldBytesSize = 8;

std::array<std::string, 3> const kFederalCode = {{"US", "SR", "FSR"}};

std::array<std::string, 60> const kStatesCode = {{
    "AL", "AK", "AZ", "AR", "CA", "CO", "CT", "DE", "DC", "FL", "GA", "HI", "ID", "IL", "IN",
    "IA", "KS", "KY", "LA", "ME", "MD", "MA", "MI", "MN", "MS", "MO", "MT", "NE", "NV", "NH",
    "NJ", "NM", "NY", "NC", "ND", "OH", "OK", "OR", "PA", "RI", "SC", "SD", "TN", "TX", "UT",
    "VT", "VA", "WA", "WV", "WI", "WY", "AS", "GU", "MP", "PR", "VI", "UM", "FM", "MH", "PW",
}};

std::array<std::string, 13> const kModifiers = {{"alt", "alternate", "bus", "business", "bypass",
                                                 "historic", "connector", "loop", "scenic", "spur",
                                                 "temporary", "toll", "truck"}};

// Shields based on a network tag in a route=road relation.
std::unordered_map<std::string, RoadShieldType> const kRoadNetworkShields = {
    // International road networks.
    {"e-road", RoadShieldType::Generic_Green},  // E 105
    {"asianhighway", RoadShieldType::Hidden},   // AH8. Blue, but usually not posted.
    // National and regional networks for some countries.
    {"ru:national", RoadShieldType::Generic_Blue},
    {"ru:regional", RoadShieldType::Generic_Blue},
    {"bg:national", RoadShieldType::Generic_Green},
    {"bg:regional", RoadShieldType::Generic_Blue},
    {"by:national", RoadShieldType::Generic_Red},
    {"by:regional", RoadShieldType::Generic_Red},
    {"co:national", RoadShieldType::Generic_White},
    {"cz:national", RoadShieldType::Generic_Red},
    {"cz:regional", RoadShieldType::Generic_Blue},
    {"ee:national", RoadShieldType::Generic_Red},
    {"ee:regional", RoadShieldType::Generic_White},
    {"fr:a-road", RoadShieldType::Generic_Red},
    {"jp:national", RoadShieldType::Generic_Blue},
    {"jp:regional", RoadShieldType::Generic_Blue},
    {"jp:prefectural", RoadShieldType::Generic_Blue},
    {"lt:national", RoadShieldType::Generic_Red},
    {"lt:regional", RoadShieldType::Generic_Blue},
    {"lv:national", RoadShieldType::Generic_Red},
    {"lv:regional", RoadShieldType::Generic_Blue},
    {"pl:national", RoadShieldType::Generic_Red},
    {"pl:regional", RoadShieldType::Generic_Orange},
    {"pl:local", RoadShieldType::Generic_White},
    {"ua:national", RoadShieldType::Generic_Blue},
    {"ua:regional", RoadShieldType::Generic_Blue},
    {"ua:territorial", RoadShieldType::Generic_White},
    {"ua:local", RoadShieldType::Generic_White},
    {"za:national", RoadShieldType::Generic_White},
    {"za:regional", RoadShieldType::Generic_White},
    // United States road networks.
    {"us:i", RoadShieldType::US_Interstate},
    {"us:us", RoadShieldType::US_Highway},
    {"us:sr", RoadShieldType::US_Highway},
    {"us:fsr", RoadShieldType::US_Highway},
};

class RoadShieldParser
{
public:
  explicit RoadShieldParser(std::string const & baseRoadNumber) : m_baseRoadNumber(baseRoadNumber) {}
  virtual ~RoadShieldParser() {}
  virtual RoadShield ParseRoadShield(std::string const & rawText) const = 0;

  RoadShieldType FindNetworkShield(std::string network) const
  {
    // Special processing for US state highways, to not duplicate the table.
    if (network.size() == 5 && strings::StartsWith(network, "US:"))
    {
      if (std::find(kStatesCode.begin(), kStatesCode.end(), network.substr(3)) != kStatesCode.end())
        return RoadShieldType::Generic_White;
    }

    // Minimum length for the network tag is 4 (US:I).
    if (network.size() >= 4)
    {
      strings::AsciiToLower(network);

      // Cut off suffixes after a semicolon repeatedly, until we find a relevant shield.
      auto semicolonPos = network.size();
      while (semicolonPos != std::string::npos)
      {
        network.resize(semicolonPos);  // cut off the ":xxx" suffix
        auto const it = kRoadNetworkShields.find(network);
        if (it != kRoadNetworkShields.cend())
          return it->second;
        semicolonPos = network.rfind(':');
      }
    }
    return RoadShieldType::Default;
  }

  std::set<RoadShield> GetRoadShields() const
  {
    std::set<RoadShield> result;
    std::set<RoadShield> defaultShields;
    std::vector<std::string> shieldsRawTests = strings::Tokenize(m_baseRoadNumber, ";");
    for (std::string const & rawText : shieldsRawTests)
    {
      RoadShield shield;
      auto slashPos = rawText.find('/');
      if (slashPos == std::string::npos)
      {
        shield = ParseRoadShield(rawText);
      }
      else
      {
        shield = ParseRoadShield(rawText.substr(slashPos + 1));
        shield.m_type = FindNetworkShield(rawText.substr(0, slashPos));
      }
      if (!shield.m_name.empty() && shield.m_type != RoadShieldType::Hidden)
      {
        if (shield.m_type != RoadShieldType::Default)
        {
          // Schedule deletion of a shield with the same text and default style, if present.
          defaultShields.insert({RoadShieldType::Default, shield.m_name, shield.m_additionalText});
        }
        result.insert(std::move(shield));
      }
    }
    for (RoadShield const & shield : defaultShields)
      result.erase(shield);
    return result;
  }

protected:
  std::string const m_baseRoadNumber;
};

class USRoadShieldParser : public RoadShieldParser
{
public:
  USRoadShieldParser(std::string const & baseRoadNumber) : RoadShieldParser(baseRoadNumber) {}
  RoadShield ParseRoadShield(std::string const & rawText) const override
  {
    std::string shieldText = rawText;

    std::replace(shieldText.begin(), shieldText.end(), '-', ' ');
    auto const shieldParts = strings::Tokenize(shieldText, " ");

    // Process long road shield titles to skip invalid data.
    if (shieldText.size() > kMaxRoadShieldBytesSize)
    {
      std::string lowerShieldText = shieldText;
      strings::AsciiToLower(lowerShieldText);

      bool modifierFound = false;
      for (auto const & modifier : kModifiers)
      {
        if (lowerShieldText.find(modifier) != std::string::npos)
        {
          modifierFound = true;
          break;
        }
      }
      if (!modifierFound)
        return RoadShield();
    }

    if (shieldParts.size() <= 1)
      return RoadShield(RoadShieldType::Default, rawText);

    std::string const & roadType =
        shieldParts[0];  // 'I' for interstates and kFederalCode/kStatesCode for highways.
    std::string roadNumber = shieldParts[1];
    std::string additionalInfo;
    if (shieldParts.size() >= 3)
    {
      additionalInfo = shieldParts[2];
      // Process cases like "US Loop 16".
      if (!strings::is_number(shieldParts[1]) && strings::is_number(shieldParts[2]))
      {
        roadNumber = shieldParts[2];
        additionalInfo = shieldParts[1];
      }
    }

    if (roadType == "I")
      return RoadShield(RoadShieldType::US_Interstate, roadNumber, additionalInfo);

    if (std::find(kFederalCode.begin(), kFederalCode.end(), shieldParts[0]) != kFederalCode.end())
      return RoadShield(RoadShieldType::US_Highway, roadNumber, additionalInfo);

    if (std::find(kStatesCode.begin(), kStatesCode.end(), shieldParts[0]) != kStatesCode.end())
      return RoadShield(RoadShieldType::Generic_White, roadNumber, additionalInfo);

    return RoadShield(RoadShieldType::Default, rawText);
  }
};

class DefaultTypeRoadShieldParser : public RoadShieldParser
{
public:
  DefaultTypeRoadShieldParser(std::string const & baseRoadNumber,
                              RoadShieldType const & defaultType)
    : RoadShieldParser(baseRoadNumber), m_type(defaultType)
  {
  }

  RoadShield ParseRoadShield(std::string const & rawText) const override
  {
    if (rawText.size() > kMaxRoadShieldBytesSize)
      return RoadShield();

    return RoadShield(m_type, rawText);
  }

private:
  RoadShieldType const m_type;
};

class SimpleRoadShieldParser : public RoadShieldParser
{
public:
  using ShieldTypes = std::unordered_map<char, RoadShieldType>;

  SimpleRoadShieldParser(std::string const & baseRoadNumber, ShieldTypes const & types)
    : RoadShieldParser(baseRoadNumber), m_types(types)
  {
  }

  RoadShield ParseRoadShield(std::string const & rawText) const override
  {
    if (rawText.size() > kMaxRoadShieldBytesSize)
      return RoadShield();

    for (auto const & p : m_types)
    {
      if (rawText.find(p.first) != std::string::npos)
        return RoadShield(p.second, rawText);
    }

    return RoadShield(RoadShieldType::Default, rawText);
  }

private:
  ShieldTypes const m_types;
};

class NumericRoadShieldParser : public RoadShieldParser
{
public:
  // A map of {lower_bound, higher_bound} -> RoadShieldType.
  using ShieldTypes =
      std::vector<std::pair<std::pair<std::uint16_t, std::uint16_t>, RoadShieldType>>;

  NumericRoadShieldParser(std::string const & baseRoadNumber, ShieldTypes const & types)
    : RoadShieldParser(baseRoadNumber), m_types(types)
  {
  }

  RoadShield ParseRoadShield(std::string const & rawText) const override
  {
    if (rawText.size() > kMaxRoadShieldBytesSize)
      return RoadShield();

    std::uint64_t ref;
    if (strings::to_uint64(rawText, ref))
    {
      for (auto const & p : m_types)
      {
        if (p.first.first <= ref && ref <= p.first.second)
          return RoadShield(p.second, rawText);
      }
    }

    return RoadShield(RoadShieldType::Default, rawText);
  }

private:
  ShieldTypes const m_types;
};

class SimpleUnicodeRoadShieldParser : public RoadShieldParser
{
public:
  struct Name
  {
    Name(std::string const & simpleName, string const & unicodeName)
      : m_simpleName(simpleName), m_unicodeName(strings::MakeUniString(unicodeName))
    {
    }

    std::string m_simpleName;
    strings::UniString m_unicodeName;
  };

  using ShieldTypes = std::vector<std::pair<Name, RoadShieldType>>;

  SimpleUnicodeRoadShieldParser(std::string const & baseRoadNumber, ShieldTypes const & types)
    : RoadShieldParser(baseRoadNumber), m_types(types)
  {
  }

  RoadShield ParseRoadShield(std::string const & rawText) const override
  {
    uint32_t constexpr kMaxRoadShieldSymbolsSize = 4 * kMaxRoadShieldBytesSize;

    if (rawText.size() > kMaxRoadShieldSymbolsSize)
      return RoadShield();

    for (auto const & p : m_types)
    {
      Name const & name = p.first;
      RoadShieldType const type = p.second;

      if (rawText.find(name.m_simpleName) != std::string::npos)
        return RoadShield(type, rawText);

      auto const rawUnicode = strings::MakeUniString(rawText);
      auto const & unicodeName = name.m_unicodeName;
      auto const it =
          std::search(rawUnicode.begin(), rawUnicode.end(), unicodeName.begin(), unicodeName.end());
      if (it != rawUnicode.end())
        return RoadShield(type, rawText);
    }

    return RoadShield(RoadShieldType::Default, rawText);
  }

private:
  ShieldTypes const m_types;
};

// Implementations of "ref" parses for some countries.

class RussiaRoadShieldParser : public DefaultTypeRoadShieldParser
{
public:
  RussiaRoadShieldParser(std::string const & baseRoadNumber)
    : DefaultTypeRoadShieldParser(baseRoadNumber, RoadShieldType::Generic_Blue)
  {
  }
};

class SpainRoadShieldParser : public DefaultTypeRoadShieldParser
{
public:
  SpainRoadShieldParser(std::string const & baseRoadNumber)
    : DefaultTypeRoadShieldParser(baseRoadNumber, RoadShieldType::Generic_Blue)
  {
  }
};

class UKRoadShieldParser : public SimpleRoadShieldParser
{
public:
  UKRoadShieldParser(std::string const & baseRoadNumber)
    : SimpleRoadShieldParser(
          baseRoadNumber, {{'M', RoadShieldType::Generic_Blue}, {'A', RoadShieldType::UK_Highway}})
  {
  }
};

class FranceRoadShieldParser : public SimpleRoadShieldParser
{
public:
  FranceRoadShieldParser(std::string const & baseRoadNumber)
    : SimpleRoadShieldParser(baseRoadNumber, {{'A', RoadShieldType::Generic_Red},
                                              {'N', RoadShieldType::Generic_Red},
                                              {'E', RoadShieldType::Generic_Green},
                                              {'D', RoadShieldType::Generic_Orange}})
  {
  }
};

class GermanyRoadShieldParser : public SimpleRoadShieldParser
{
public:
  GermanyRoadShieldParser(std::string const & baseRoadNumber)
    : SimpleRoadShieldParser(baseRoadNumber, {{'A', RoadShieldType::Generic_Blue},
                                              {'B', RoadShieldType::Generic_Orange},
                                              {'L', RoadShieldType::Generic_White},
                                              {'K', RoadShieldType::Generic_White}})
  {
  }
};

class UkraineRoadShieldParser : public SimpleUnicodeRoadShieldParser
{
public:
  // The second parameter in the constructor is a cyrillic symbol.
  UkraineRoadShieldParser(std::string const & baseRoadNumber)
    : SimpleUnicodeRoadShieldParser(baseRoadNumber,
                                    {{Name("M", "М"), RoadShieldType::Generic_Blue},
                                     {Name("H", "Н"), RoadShieldType::Generic_Blue},
                                     {Name("P", "Р"), RoadShieldType::Generic_Blue},
                                     {Name("E", "Е"), RoadShieldType::Generic_Green}})
  {
  }
};

class BelarusRoadShieldParser : public SimpleUnicodeRoadShieldParser
{
public:
  // The second parameter in the constructor is a cyrillic symbol.
  BelarusRoadShieldParser(std::string const & baseRoadNumber)
    : SimpleUnicodeRoadShieldParser(baseRoadNumber,
                                    {{Name("M", "М"), RoadShieldType::Generic_Red},
                                     {Name("P", "Р"), RoadShieldType::Generic_Red},
                                     {Name("E", "Е"), RoadShieldType::Generic_Green}})
  {
  }
};

class LatviaRoadShieldParser : public SimpleRoadShieldParser
{
public:
  LatviaRoadShieldParser(std::string const & baseRoadNumber)
    : SimpleRoadShieldParser(baseRoadNumber, {{'A', RoadShieldType::Generic_Red},
                                              {'E', RoadShieldType::Generic_Green},
                                              {'P', RoadShieldType::Generic_Blue}})
  {
  }
};

class NetherlandsRoadShieldParser : public SimpleRoadShieldParser
{
public:
  NetherlandsRoadShieldParser(std::string const & baseRoadNumber)
    : SimpleRoadShieldParser(baseRoadNumber, {{'A', RoadShieldType::Generic_Red},
                                              {'E', RoadShieldType::Generic_Green},
                                              {'N', RoadShieldType::Generic_Orange}})
  {
  }
};

class FinlandRoadShieldParser : public NumericRoadShieldParser
{
public:
  FinlandRoadShieldParser(std::string const & baseRoadNumber)
    : NumericRoadShieldParser(baseRoadNumber, {{{1, 30}, RoadShieldType::Generic_Red},
                                               {{40, 99}, RoadShieldType::Generic_Orange},
                                               {{100, 999}, RoadShieldType::Generic_White},
                                               {{1000, 9999}, RoadShieldType::Generic_Blue},
                                               {{10000, 60000}, RoadShieldType::Hidden}})
  {
  }
};

class EstoniaRoadShieldParser : public NumericRoadShieldParser
{
public:
  EstoniaRoadShieldParser(std::string const & baseRoadNumber)
    : NumericRoadShieldParser(baseRoadNumber, {{{1, 11}, RoadShieldType::Generic_Red},
                                               {{12, 91}, RoadShieldType::Generic_Orange},
                                               {{92, 92}, RoadShieldType::Generic_Red},
                                               {{93, 95}, RoadShieldType::Generic_Orange},
                                               {{96, 999}, RoadShieldType::Generic_White},
                                               {{1000, 60000}, RoadShieldType::Hidden}})
  {
  }
};
}  // namespace

namespace ftypes
{
std::set<RoadShield> GetRoadShields(FeatureType & f)
{
  std::string const roadNumber = f.GetRoadNumber();
  if (roadNumber.empty())
    return std::set<RoadShield>();

  // Find out country name.
  std::string mwmName = f.GetID().GetMwmName();
  ASSERT_NOT_EQUAL(mwmName, FeatureID::kInvalidFileName, ());
  auto const underlinePos = mwmName.find('_');
  if (underlinePos != std::string::npos)
    mwmName = mwmName.substr(0, underlinePos);

  if (mwmName == "US")
    return USRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "UK")
    return UKRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Russia")
    return RussiaRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "France")
    return FranceRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Germany")
    return GermanyRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Spain")
    return SpainRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Ukraine")
    return UkraineRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Belarus")
    return BelarusRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Latvia")
    return LatviaRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Netherlands")
    return NetherlandsRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Finland")
    return FinlandRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Estonia")
    return EstoniaRoadShieldParser(roadNumber).GetRoadShields();

  return SimpleRoadShieldParser(roadNumber, SimpleRoadShieldParser::ShieldTypes()).GetRoadShields();
}

std::string DebugPrint(RoadShieldType shieldType)
{
  using ftypes::RoadShieldType;
  switch (shieldType)
  {
  case RoadShieldType::Default: return "default";
  case RoadShieldType::Generic_White: return "white";
  case RoadShieldType::Generic_Blue: return "blue";
  case RoadShieldType::Generic_Green: return "green";
  case RoadShieldType::Generic_Orange: return "orange";
  case RoadShieldType::Generic_Red: return "red";
  case RoadShieldType::US_Interstate: return "US interstate";
  case RoadShieldType::US_Highway: return "US highway";
  case RoadShieldType::UK_Highway: return "UK highway";
  case RoadShieldType::Hidden: return "hidden";
  case RoadShieldType::Count: CHECK(false, ("RoadShieldType::Count is not to be used as a type"));
  }
  return string();
}

std::string DebugPrint(RoadShield const & shield)
{
  return DebugPrint(shield.m_type) + "/" + shield.m_name +
         (shield.m_additionalText.empty() ? "" : " (" + shield.m_additionalText + ")");
}
}  // namespace ftypes
