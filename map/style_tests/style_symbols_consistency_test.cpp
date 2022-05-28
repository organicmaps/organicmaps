#include "testing/testing.hpp"

#include "map/style_tests/helpers.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/drawing_rules.hpp"
#include "indexer/drules_include.hpp"
#include "indexer/map_style_reader.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"

#include "coding/parse_xml.hpp"
#include "coding/reader.hpp"

#include <set>
#include <string>
#include <vector>

using namespace std;

namespace style_symbols_consistency_tests
{
class SdfParsingDispatcher
{
public:
  explicit SdfParsingDispatcher(set<string> & symbols)
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
  drule::rules().ForEachRule([&symbols](drule::BaseRule const * rule)
  {
    SymbolRuleProto const * symbol = rule->GetSymbol();
    if (symbol && !symbol->name().empty())
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

// Tests that all symbols specified in drawing rules have corresponding symbols in resources
UNIT_TEST(Test_SymbolsConsistency)
{
  bool res = true;

  string const densities[] = { "mdpi", "hdpi", "xhdpi", "xxhdpi", "xxxhdpi", "6plus" };

  styles::RunForEveryMapStyle([&](MapStyle mapStyle)
  {
    set<string> const drawingRuleSymbols = GetSymbolsSetFromDrawingRule();

    for (string const & density : densities)
    {
      set<string> const resourceStyles = GetSymbolsSetFromResourcesFile(density);

      vector<string> missed;
      set_difference(drawingRuleSymbols.begin(), drawingRuleSymbols.end(),
                     resourceStyles.begin(), resourceStyles.end(),
                     back_inserter(missed));

      if (!missed.empty())
      {
        // We are interested in all set of bugs, therefore we do not stop test here but
        // continue it just keeping in res that test failed.
        LOG(LINFO, ("Symbols mismatch: style", mapStyle, ", density", density, ", missed", missed));
        res = false;
      }
    }
  });

  TEST(res, ());
}
}  // namespace style_symbols_consistency_tests
