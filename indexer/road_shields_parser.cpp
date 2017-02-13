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

std::array<std::string, 63> const kStatesCode = {{
    "US", "SR", "FSR", "AL", "AK", "AZ", "AR", "CA", "CO", "CT", "DE", "DC", "FL", "GA", "HI", "ID",
    "IL", "IN", "IA",  "KS", "KY", "LA", "ME", "MD", "MA", "MI", "MN", "MS", "MO", "MT", "NE", "NV",
    "NH", "NJ", "NM",  "NY", "NC", "ND", "OH", "OK", "OR", "PA", "RI", "SC", "SD", "TN", "TX", "UT",
    "VT", "VA", "WA",  "WV", "WI", "WY", "AS", "GU", "MP", "PR", "VI", "UM", "FM", "MH", "PW",
}};

std::array<std::string, 13> const kModifiers = {{
    "alt",  "alternate", "bus",  "business",  "bypass", "historic", "connector",
    "loop", "scenic",    "spur", "temporary", "toll",   "truck"}};

class RoadShieldParser
{
public:
  RoadShieldParser(std::string const & baseRoadNumber)
  : m_baseRoadNumber(baseRoadNumber)
  {}
  virtual ~RoadShieldParser(){}
  virtual RoadShield ParseRoadShield(std::string const & rawText) = 0;

  std::vector<RoadShield> GetRoadShields()
  {
    std::vector<RoadShield> result;
    std::vector<std::string> shieldsRawTests = strings::Tokenize(m_baseRoadNumber, ";");
    for (std::string const & rawText : shieldsRawTests)
    {
      RoadShield shield = ParseRoadShield(rawText);
      if (!shield.m_name.empty())
        result.push_back(std::move(shield));
    }
    return result;
  }

protected:
  std::string const m_baseRoadNumber;
};

class USRoadShieldParser : public RoadShieldParser
{
public:
  USRoadShieldParser(std::string const & baseRoadNumber)
  : RoadShieldParser(baseRoadNumber)
  {}

  RoadShield ParseRoadShield(std::string const & rawText) override
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

    std::string const & roadType = shieldParts[0]; // 'I' for interstates and kStatesCode for highways.
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

    if (std::find(kStatesCode.begin(), kStatesCode.end(), shieldParts[0]) != kStatesCode.end())
      return RoadShield(RoadShieldType::US_Highway, roadNumber, additionalInfo);

    return RoadShield(RoadShieldType::Default, rawText);
  }
};

class SimpleRoadShieldParser : public RoadShieldParser
{
public:
  using ShieldTypes = std::unordered_map<char, RoadShieldType>;

  SimpleRoadShieldParser(std::string const & baseRoadNumber, ShieldTypes const & types)
  : RoadShieldParser(baseRoadNumber)
  , m_types(types)
  {}

  RoadShield ParseRoadShield(std::string const & rawText) override
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

class UKRoadShieldParser : public SimpleRoadShieldParser
{
public:
  UKRoadShieldParser(std::string const & baseRoadNumber)
  : SimpleRoadShieldParser(baseRoadNumber, {{'M', RoadShieldType::Generic_Blue}, {'A', RoadShieldType::UK_Highway}})
  {}
};

class RussiaRoadShieldParser : public RoadShieldParser
{
public:
  RussiaRoadShieldParser(std::string const & baseRoadNumber)
  : RoadShieldParser(baseRoadNumber)
  {}

  RoadShield ParseRoadShield(std::string const & rawText) override
  {
    if (rawText.size() > kMaxRoadShieldBytesSize)
      return RoadShield();

    strings::UniString s = strings::MakeUniString(rawText);
    if (s[0] == 'E' || s[0] == strings::UniChar(1045))  // Latin and cyrillic.
      return RoadShield(RoadShieldType::Generic_Green, rawText);

    return RoadShield(RoadShieldType::Generic_Blue, rawText);
  }
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

  RoadShield ParseRoadShield(std::string const & rawText) override
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
      auto const it = std::search(rawUnicode.begin(), rawUnicode.end(), unicodeName.begin(), unicodeName.end());
      if (it != rawUnicode.end())
        return RoadShield(type, rawText);
    }

    return RoadShield(RoadShieldType::Default, rawText);
  }

private:
  ShieldTypes const m_types;
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

class UkraineRoadShieldParser : public SimpleUnicodeRoadShieldParser
{
public:
  // The second parameter in the constructor is a cyrillic symbol.
  UkraineRoadShieldParser(std::string const & baseRoadNumber)
  : SimpleUnicodeRoadShieldParser(baseRoadNumber, {{Name("M", "М"), RoadShieldType::Generic_Blue},
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
  : SimpleUnicodeRoadShieldParser(baseRoadNumber, {{Name("M", "М"), RoadShieldType::Generic_Red},
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
  {}
};
}  // namespace

namespace ftypes
{
std::vector<RoadShield> GetRoadShields(FeatureType const & f)
{
  std::string const roadNumber = f.GetRoadNumber();
  if (roadNumber.empty())
    return std::vector<RoadShield>();

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
  if (mwmName == "Ukraine")
    return UkraineRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Belarus")
    return BelarusRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Latvia")
    return LatviaRoadShieldParser(roadNumber).GetRoadShields();
  if (mwmName == "Netherlands")
    return NetherlandsRoadShieldParser(roadNumber).GetRoadShields();
  
  return std::vector<RoadShield>{RoadShield(RoadShieldType::Default, roadNumber)};
}
}  // namespece ftypes
