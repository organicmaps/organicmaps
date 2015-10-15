#include "testing/testing.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/drawing_rules.hpp"
#include "indexer/drules_include.hpp"
#include "indexer/map_style_reader.hpp"

#include "graphics/defines.hpp"

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

}  // namespace

UNIT_TEST(Test_SymbolsConsistency)
{
  // Tests that all symbols specified in drawing rules have corresponding symbols in resources

  bool res = true;

  for (size_t s = 0; s < MapStyleCount; ++s)
  {
    MapStyle const mapStyle = static_cast<MapStyle>(s);

    GetStyleReader().SetCurrentStyle(mapStyle);
    classificator::Load();

    unordered_set<string> const drawingRuleSymbols = GetSymbolsSetFromDrawingRule();

    for (size_t d = 0; d < graphics::EDensityCount; ++d)
    {
      string const density = graphics::convert(static_cast<graphics::EDensity>(d));

      unordered_set<string> const resourceStyles = GetSymbolsSetFromResourcesFile(density);

      unordered_set<string> const s = Subtract(drawingRuleSymbols, resourceStyles);

      vector<string> const missed(s.begin(), s.end());

      if (!missed.empty())
      {
        // We are interested in all set of bugs, therefore we do not stop test here but
        // continue it just keep in res that test failed.
        LOG(LINFO, ("Symbols mismatch: style", mapStyle, ", density", density, ", missed", missed));
        res = false;
      }
    }
  }

  TEST(res, ());
}
