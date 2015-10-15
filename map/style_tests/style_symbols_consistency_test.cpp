#include "testing/testing.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/drawing_rules.hpp"
#include "indexer/drules_include.hpp"
#include "indexer/map_style_reader.hpp"

#include "coding/reader.hpp"
#include "coding/parse_xml.hpp"

#include "base/logging.hpp"

#include "std/array.hpp"
#include "std/string.hpp"
#include "std/unordered_set.hpp"

namespace
{

class SdfParsingDispatcher
{
public:
  SdfParsingDispatcher(unordered_set<string> & symbols)
      : m_symbols(symbols)
  {}

  bool Push(string const &) { return true; }
  void Pop(string const &) {}
  void CharData(string const &) {}
  void AddAttr(string const & attribute, string const & value)
  {
    if (attribute == "name")
      m_symbols.insert(value);
  }

private:
  unordered_set<string> & m_symbols;
};

unordered_set<string> GetSymbolsSetFromDrawingRule()
{
  unordered_set<string> symbols;
  drule::rules().ForEachRule([&symbols](int, int, int, drule::BaseRule const * rule)
  {
    SymbolRuleProto const * const symbol = rule->GetSymbol();
    if (nullptr != symbol && symbol->has_name())
      symbols.insert(symbol->name());
  });
  return symbols;
}

unordered_set<string> GetSymbolsSetFromResourcesFile(string const & density)
{
  unordered_set<string> symbols;
  SdfParsingDispatcher dispatcher(symbols);
  ReaderPtr<Reader> reader = GetStyleReader().GetResourceReader("symbols.sdf", density);
  ReaderSource<ReaderPtr<Reader> > source(reader);
  ParseXML(source, dispatcher);
  return symbols;
}

// returns s1 - s2
unordered_set<string> Subtract(unordered_set<string> const & s1, unordered_set<string> const & s2)
{
  unordered_set<string> res(s1);
  for (auto const & s : s2)
    res.erase(s);
  return res;
}

array<MapStyle, 3> g_styles =
{{
  MapStyleLight,
  MapStyleDark,
  MapStyleClear
}};

array<string, 6> g_densities =
{{
  "ldpi",
  "mdpi",
  "hdpi",
  "xhdpi",
  "xxhdpi",
  "6plus"
}};

}  // namespace

UNIT_TEST(Test_SymbolsConsistency)
{
  bool res = true;

  for (auto const style : g_styles)
  {
    GetStyleReader().SetCurrentStyle(style);
    classificator::Load();

    unordered_set<string> const drawingRuleSymbols = GetSymbolsSetFromDrawingRule();

    for (auto const & density : g_densities)
    {
      unordered_set<string> const resourceStyles = GetSymbolsSetFromResourcesFile(density);

      unordered_set<string> const s = Subtract(drawingRuleSymbols, resourceStyles);

      vector<string> const missed(s.begin(), s.end());

      if (!missed.empty())
      {
        LOG(LINFO, ("Symbols mismatch: style", style, ", density", density, ", missed", missed));
        res = false;
      }
    }
  }

  TEST(res, ());
}
