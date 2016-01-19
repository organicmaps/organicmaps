#include "testing/testing.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/drawing_rules.hpp"
#include "indexer/drules_include.hpp"
#include "indexer/map_style_reader.hpp"

#include "base/logging.hpp"

#include "coding/parse_xml.hpp"
#include "coding/reader.hpp"

#include "std/algorithm.hpp"
#include "std/set.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

namespace
{

class SdfParsingDispatcher
{
public:
  SdfParsingDispatcher(set<string> & symbols)
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
  set<string> & m_symbols;
};

set<string> GetSymbolsSetFromDrawingRule()
{
  set<string> symbols;
  drule::rules().ForEachRule([&symbols](int, int, int, drule::BaseRule const * rule)
  {
    SymbolRuleProto const * const symbol = rule->GetSymbol();
    if (nullptr != symbol && symbol->has_name())
      symbols.insert(symbol->name());
  });
  return symbols;
}

set<string> GetSymbolsSetFromResourcesFile(string const & density)
{
  set<string> symbols;
  SdfParsingDispatcher dispatcher(symbols);
  ReaderPtr<Reader> reader = GetStyleReader().GetResourceReader("symbols.sdf", density);
  ReaderSource<ReaderPtr<Reader> > source(reader);
  ParseXML(source, dispatcher);
  return symbols;
}

}  // namespace

UNIT_TEST(Test_SymbolsConsistency)
{
  // Tests that all symbols specified in drawing rules have corresponding symbols in resources

  bool res = true;

  vector<string> densities = { "ldpi", "mdpi", "hdpi", "xhdpi", "xxhdpi", "6plus" };

  for (size_t s = 0; s < MapStyleCount; ++s)
  {
    MapStyle const mapStyle = static_cast<MapStyle>(s);
    if (mapStyle == MapStyleMerged)
      continue;

    GetStyleReader().SetCurrentStyle(mapStyle);
    classificator::Load();

    set<string> const drawingRuleSymbols = GetSymbolsSetFromDrawingRule();

    for (size_t d = 0; d < densities.size(); ++d)
    {
      set<string> const resourceStyles = GetSymbolsSetFromResourcesFile(densities[d]);

      vector<string> missed;
      set_difference(drawingRuleSymbols.begin(), drawingRuleSymbols.end(),
                     resourceStyles.begin(), resourceStyles.end(),
                     back_inserter(missed));

      if (!missed.empty())
      {
        // We are interested in all set of bugs, therefore we do not stop test here but
        // continue it just keeping in res that test failed.
        LOG(LINFO, ("Symbols mismatch: style", mapStyle, ", density", densities[d], ", missed", missed));
        res = false;
      }
    }
  }

  TEST(res, ());
}
