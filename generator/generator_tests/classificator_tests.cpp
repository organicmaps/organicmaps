#include "../../testing/testing.hpp"

#include "../../indexer/classificator.hpp"
#include "../../indexer/classificator_loader.hpp"
#include "../../indexer/feature_visibility.hpp"
#include "../../indexer/feature_data.hpp"

#include "../../base/logging.hpp"


namespace
{
  class DoCheckConsistency
  {
    Classificator const & m_c;
  public:
    DoCheckConsistency(Classificator const & c) : m_c(c) {}
    void operator() (ClassifObject const * p, uint32_t type) const
    {
      if (p->IsDrawableAny() && !m_c.IsTypeValid(type))
        TEST(false, ("Inconsistency type", type, m_c.GetFullObjectName(type)));
    }
  };
}

UNIT_TEST(Classificator_CheckConsistency)
{
  classificator::Load();
  Classificator const & c = classif();

  DoCheckConsistency doCheck(c);
  c.ForEachTree(doCheck);
}

using namespace feature;

namespace
{

class DoCheckStyles
{
  Classificator const & m_c;
  EGeomType m_geomType;
  int m_rules;

public:
  DoCheckStyles(Classificator const & c, EGeomType geomType, int rules)
    : m_c(c), m_geomType(geomType), m_rules(rules)
  {
  }

  void operator() (ClassifObject const * p, uint32_t type) const
  {
    if (p->IsDrawableAny())
    {
      TypesHolder holder(m_geomType);
      holder(type);

      pair<int, int> const range = GetDrawableScaleRangeForRules(holder, m_rules);
      if (range.first == -1 || range.second == -1)
        LOG(LINFO, ("No styles:", type, m_c.GetFullObjectName(type)));
    }
  }
};

void ForEachObject(Classificator const & c, string const & name,
                   EGeomType geomType, int rules)
{
  vector<string> v;
  v.push_back(name);
  uint32_t const type = c.GetTypeByPath(v);

  DoCheckStyles doCheck(c, geomType, rules);
  c.GetObject(type)->ForEachObjectInTree(doCheck, type);
}

void CheckPointStyles(Classificator const & c, string const & name)
{
  ForEachObject(c, name, GEOM_POINT, RULE_CAPTION | RULE_SYMBOL);
}

void CheckLineStyles(Classificator const & c, string const & name)
{
  ForEachObject(c, name, GEOM_LINE, RULE_PATH_TEXT);
}

}

UNIT_TEST(Classificator_DrawingRules)
{
  classificator::Load();
  Classificator const & c = classif();

  CheckPointStyles(c, "landuse");
  CheckPointStyles(c, "amenity");
  CheckPointStyles(c, "historic");
  CheckPointStyles(c, "office");
  CheckPointStyles(c, "place");
  CheckPointStyles(c, "shop");
  CheckPointStyles(c, "sport");
  CheckPointStyles(c, "tourism");

  CheckLineStyles(c, "highway");
  CheckLineStyles(c, "waterway");
  //CheckLineStyles(c, "railway");
}
