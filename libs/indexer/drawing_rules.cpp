#include "indexer/drawing_rules.hpp"
#include "indexer/classificator.hpp"
#include "indexer/drules_include.hpp"
#include "indexer/map_style_reader.hpp"
#include "indexer/scales.hpp"

#include "defines.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"

#include <functional>

#include <boost/iterator/iterator_facade.hpp>

#include <google/protobuf/text_format.h>

namespace drule
{
using namespace std;

namespace
{
uint32_t const DEFAULT_BG_COLOR = 0xEEEEDD;
}  // namespace

LineRuleProto const * BaseRule::GetLine() const
{
  return 0;
}

AreaRuleProto const * BaseRule::GetArea() const
{
  return 0;
}

SymbolRuleProto const * BaseRule::GetSymbol() const
{
  return 0;
}

CaptionRuleProto const * BaseRule::GetCaption() const
{
  return 0;
}

PathTextRuleProto const * BaseRule::GetPathtext() const
{
  return 0;
}

ShieldRuleProto const * BaseRule::GetShield() const
{
  return nullptr;
}

bool BaseRule::TestFeature(FeatureType & ft, int zoom) const
{
  if (nullptr == m_selector)
    return true;
  return m_selector->Test(ft, zoom);
}

void BaseRule::SetSelector(unique_ptr<ISelector> && selector)
{
  m_selector = std::move(selector);
}

RulesHolder::RulesHolder() : m_bgColors(scales::UPPER_STYLE_SCALE + 1, DEFAULT_BG_COLOR) {}

RulesHolder::~RulesHolder()
{
  Clean();
}

void RulesHolder::Clean()
{
  for (size_t i = 0; i < m_dRules.size(); ++i)
    delete m_dRules[i];

  m_dRules.clear();
  m_colors.clear();
}

Key RulesHolder::AddRule(int scale, TypeT type, BaseRule * p)
{
  ASSERT(0 <= scale && scale <= scales::GetUpperStyleScale(), (scale));
  ASSERT(0 <= type && type < count_of_rules, ());

  m_dRules.push_back(p);
  auto const index = m_dRules.size() - 1;
  Key const k(scale, type, index);

  ASSERT(Find(k) == p, (index));
  return k;
}

BaseRule const * RulesHolder::Find(Key const & k) const
{
  ASSERT_LESS(k.m_index, m_dRules.size(), ());
  return m_dRules[k.m_index];
}

uint32_t RulesHolder::GetBgColor(int scale) const
{
  ASSERT_LESS(scale, static_cast<int>(m_bgColors.size()), ());
  ASSERT_GREATER_OR_EQUAL(scale, 0, ());
  return m_bgColors[scale];
}

uint32_t RulesHolder::GetColor(std::string const & name) const
{
  auto const it = m_colors.find(name);
  if (it == m_colors.end())
  {
    LOG(LWARNING, ("Requested color '" + name + "' is not found"));
    return 0;
  }
  return it->second;
}

namespace
{
RulesHolder & rules(MapStyle mapStyle)
{
  static RulesHolder h[MapStyleCount];
  return h[mapStyle];
}
}  // namespace

RulesHolder & rules()
{
  return rules(GetStyleReader().GetCurrentStyle());
}

namespace
{
namespace proto_rules
{
class Line : public BaseRule
{
  LineRuleProto m_line;

public:
  explicit Line(LineRuleProto const & r) : m_line(r)
  {
    ASSERT(r.has_pathsym() || r.width() > 0, ("Zero width line w/o path symbol)"));
  }

  virtual LineRuleProto const * GetLine() const { return &m_line; }
};

class Area : public BaseRule
{
  AreaRuleProto m_area;

public:
  explicit Area(AreaRuleProto const & r) : m_area(r) {}

  virtual AreaRuleProto const * GetArea() const { return &m_area; }
};

class Symbol : public BaseRule
{
  SymbolRuleProto m_symbol;

public:
  explicit Symbol(SymbolRuleProto const & r) : m_symbol(r) {}

  virtual SymbolRuleProto const * GetSymbol() const { return &m_symbol; }
};

class Caption : public BaseRule
{
  CaptionRuleProto m_caption;

public:
  explicit Caption(CaptionRuleProto const & r) : m_caption(r) { ASSERT(r.has_primary(), ()); }

  virtual CaptionRuleProto const * GetCaption() const { return &m_caption; }
};

class PathText : public BaseRule
{
  PathTextRuleProto m_pathtext;

public:
  explicit PathText(PathTextRuleProto const & r) : m_pathtext(r) {}

  virtual PathTextRuleProto const * GetPathtext() const { return &m_pathtext; }
};

class Shield : public BaseRule
{
  ShieldRuleProto m_shield;

public:
  explicit Shield(ShieldRuleProto const & r) : m_shield(r) {}

  virtual ShieldRuleProto const * GetShield() const { return &m_shield; }
};
}  // namespace proto_rules

class DoSetIndex
{
public:
  ContainerProto m_cont;

private:
  vector<string> m_names;

  typedef ClassifElementProto ElementT;

  class RandI : public boost::iterator_facade<RandI, ElementT const, boost::random_access_traversal_tag>
  {
    ContainerProto const * m_cont;

  public:
    int m_index;

    RandI() : m_cont(0), m_index(-1) {}
    RandI(ContainerProto const & cont, int ind) : m_cont(&cont), m_index(ind) {}

    ElementT const & dereference() const { return m_cont->cont(m_index); }
    bool equal(RandI const & r) const { return m_index == r.m_index; }
    void increment() { ++m_index; }
    void decrement() { --m_index; }
    void advance(size_t n) { m_index += n; }
    difference_type distance_to(RandI const & r) const { return (r.m_index - m_index); }
  };

  struct less_name
  {
    bool operator()(ElementT const & e1, ElementT const & e2) const { return (e1.name() < e2.name()); }
    bool operator()(string const & e1, ElementT const & e2) const { return (e1 < e2.name()); }
    bool operator()(ElementT const & e1, string const & e2) const { return (e1.name() < e2); }
  };

  int FindIndex() const
  {
    string name = m_names[0];
    for (size_t i = 1; i < m_names.size(); ++i)
      name = name + "-" + m_names[i];

    int const count = m_cont.cont_size();
    int const i = lower_bound(RandI(m_cont, 0), RandI(m_cont, count), name, less_name()).m_index;
    ASSERT_GREATER_OR_EQUAL(i, 0, ());
    ASSERT_LESS_OR_EQUAL(i, count, ());

    if (i < count && m_cont.cont(i).name() == name)
      return i;
    else
      return -1;
  }

  RulesHolder & m_holder;

  template <class TRule, class TProtoRule>
  void AddRule(ClassifObject * p, int scale, TypeT type, TProtoRule const & rule, vector<string> const & apply_if)
  {
    unique_ptr<ISelector> selector;
    if (!apply_if.empty())
    {
      selector = ParseSelector(apply_if);
      if (selector == nullptr)
      {
        LOG(LERROR, ("Runtime selector has not been created:", apply_if));
        return;
      }
    }

    BaseRule * obj = new TRule(rule);
    obj->SetSelector(std::move(selector));
    Key k = m_holder.AddRule(scale, type, obj);
    p->SetVisibilityOnScale(true, scale);
    k.SetPriority(rule.priority());
    p->AddDrawRule(k);
  }

  static void DrawElementGetApplyIf(DrawElementProto const & de, vector<string> & apply_if)
  {
    apply_if.clear();
    apply_if.reserve(de.apply_if_size());
    for (int i = 0; i < de.apply_if_size(); ++i)
      apply_if.emplace_back(de.apply_if(i));
  }

public:
  explicit DoSetIndex(RulesHolder & holder) : m_holder(holder) {}

  void operator()(ClassifObject * p)
  {
    m_names.push_back(p->GetName());

    int const i = FindIndex();
    if (i != -1)
    {
      vector<string> apply_if;

      ClassifElementProto const & ce = m_cont.cont(i);
      for (int j = 0; j < ce.element_size(); ++j)
      {
        DrawElementProto const & de = ce.element(j);

        using namespace proto_rules;

        DrawElementGetApplyIf(de, apply_if);

        for (int k = 0; k < de.lines_size(); ++k)
          AddRule<Line>(p, de.scale(), line, de.lines(k), apply_if);

        if (de.has_area())
          AddRule<Area>(p, de.scale(), area, de.area(), apply_if);

        if (de.has_symbol())
          AddRule<Symbol>(p, de.scale(), symbol, de.symbol(), apply_if);

        if (de.has_caption())
          AddRule<Caption>(p, de.scale(), caption, de.caption(), apply_if);

        if (de.has_path_text())
          AddRule<PathText>(p, de.scale(), pathtext, de.path_text(), apply_if);

        if (de.has_shield())
          AddRule<Shield>(p, de.scale(), shield, de.shield(), apply_if);
      }
    }

    p->ForEachObject(ref(*this));

    m_names.pop_back();
  }
};
}  // namespace

void RulesHolder::InitBackgroundColors(ContainerProto const & cont)
{
  // WARNING!
  // Background color is not specified in current format.
  // Therefore, we use color of "natural-land" area element as background color.
  // If such element is not present or if the element is not area then we use default background color

  // Default background color is any found color, it is used if color is not specified for scale
  uint32_t bgColorDefault = DEFAULT_BG_COLOR;

  // Background colors specified for scales
  unordered_map<int, uint32_t> bgColorForScale;

  // Find the "natural-land" classification element
  for (int i = 0; i < cont.cont_size(); ++i)
  {
    ClassifElementProto const & ce = cont.cont(i);
    if (ce.name() == "natural-land")
    {
      // Take any area draw element
      for (int j = 0; j < ce.element_size(); ++j)
      {
        DrawElementProto const & de = ce.element(j);
        if (de.has_area())
        {
          // Take the color of the draw element
          AreaRuleProto const & rule = de.area();
          bgColorDefault = rule.color();

          if (de.scale() != 0)
            bgColorForScale.insert(make_pair(de.scale(), rule.color()));
        }
      }
      break;
    }
  }

  ASSERT_EQUAL(m_bgColors.size(), scales::UPPER_STYLE_SCALE + 1, ());
  for (int scale = 0; scale <= scales::UPPER_STYLE_SCALE; ++scale)
  {
    auto const i = bgColorForScale.find(scale);
    if (bgColorForScale.end() != i)
      m_bgColors[scale] = i->second;
    else
      m_bgColors[scale] = bgColorDefault;
  }
}

void RulesHolder::InitColors(ContainerProto const & cp)
{
  if (!cp.has_colors())
    return;

  ASSERT_EQUAL(m_colors.size(), 0, ());
  for (int i = 0; i < cp.colors().value_size(); i++)
  {
    ColorElementProto const & proto = cp.colors().value(i);
    m_colors.insert(std::make_pair(proto.name(), proto.color()));
  }
}

void RulesHolder::LoadFromBinaryProto(string const & s)
{
  Clean();

  DoSetIndex doSet(*this);

  CHECK(doSet.m_cont.ParseFromString(s), ("Error in proto loading!"));

  classif().GetMutableRoot()->ForEachObject(ref(doSet));

  InitBackgroundColors(doSet.m_cont);
  InitColors(doSet.m_cont);
}

void LoadRules()
{
  string buffer;
  GetStyleReader().GetDrawingRulesReader().ReadAsString(buffer);
  rules().LoadFromBinaryProto(buffer);
}

}  // namespace drule
