#include "indexer/drawing_rules.hpp"
#include "indexer/classificator.hpp"
#include "indexer/drules_include.hpp"
#include "indexer/map_style_reader.hpp"
#include "indexer/scales.hpp"

#include "defines.hpp"

#include "platform/platform.hpp"

#include "base/logging.hpp"

#include "std/bind.hpp"
#include "std/iterator_facade.hpp"
#include "std/unordered_map.hpp"

#include <google/protobuf/text_format.h>

namespace
{
  uint32_t const DEFAULT_BG_COLOR = 0xEEEEDD;
}

namespace drule {

BaseRule::BaseRule() : m_type(node | way)
{}

BaseRule::~BaseRule()
{}

void BaseRule::CheckCacheSize(size_t s)
{
  m_id1.resize(s);
  MakeEmptyID();
}

uint32_t BaseRule::GetID(size_t threadSlot) const
{
  ASSERT(m_id1.size() > threadSlot, ());
  return m_id1[threadSlot];
}

void BaseRule::SetID(size_t threadSlot, uint32_t id) const
{
  ASSERT(m_id1.size() > threadSlot, ());
  m_id1[threadSlot] = id;
}

void BaseRule::MakeEmptyID(size_t threadSlot)
{
  ASSERT(m_id1.size() > threadSlot, ());
  m_id1[threadSlot] = empty_id;
}

void BaseRule::MakeEmptyID()
{
  for (size_t i = 0; i < m_id1.size(); ++i)
    MakeEmptyID(i);
}

LineDefProto const * BaseRule::GetLine() const
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

CaptionDefProto const * BaseRule::GetCaption(int) const
{
  return 0;
}

CircleRuleProto const * BaseRule::GetCircle() const
{
  return 0;
}

ShieldRuleProto const * BaseRule::GetShield() const
{
  return nullptr;
}

bool BaseRule::TestFeature(FeatureType const & ft, int /* zoom */) const
{
  if (nullptr == m_selector)
    return true;
  return m_selector->Test(ft);
}

void BaseRule::SetSelector(unique_ptr<ISelector> && selector)
{
  m_selector = move(selector);
}

RulesHolder::RulesHolder()
  : m_bgColors(scales::UPPER_STYLE_SCALE+1, DEFAULT_BG_COLOR)
  , m_cityRankTable(GetConstRankCityRankTable())
{}

RulesHolder::~RulesHolder()
{
  Clean();
}

void RulesHolder::Clean()
{
  for (size_t i = 0; i < m_container.size(); ++i)
  {
    rule_vec_t & v = m_container[i];
    for (size_t j = 0; j < v.size(); ++j)
      delete v[j];
    v.clear();
  }

  m_rules.clear();
}

Key RulesHolder::AddRule(int scale, rule_type_t type, BaseRule * p)
{
  ASSERT ( 0 <= scale && scale <= scales::GetUpperStyleScale(), (scale) );
  ASSERT ( 0 <= type && type < count_of_rules, () );

  m_container[type].push_back(p);

  vector<uint32_t> & v = m_rules[scale][type];
  v.push_back(static_cast<uint32_t>(m_container[type].size()-1));

  int const ret = static_cast<int>(v.size() - 1);
  Key k(scale, type, ret);
  ASSERT ( Find(k) == p, (ret) );
  return k;
}

BaseRule const * RulesHolder::Find(Key const & k) const
{
  rules_map_t::const_iterator i = m_rules.find(k.m_scale);
  if (i == m_rules.end()) return 0;

  vector<uint32_t> const & v = (i->second)[k.m_type];

  ASSERT ( k.m_index >= 0, (k.m_index) );
  if (static_cast<size_t>(k.m_index) < v.size())
    return m_container[k.m_type][v[k.m_index]];
  else
    return 0;
}

uint32_t RulesHolder::GetBgColor(int scale) const
{
  ASSERT_LESS(scale, m_bgColors.size(), ());
  ASSERT_GREATER_OR_EQUAL(scale, 0, ());
  return m_bgColors[scale];
}

double RulesHolder::GetCityRank(int scale, uint32_t population) const
{
  double rank;
  if (!m_cityRankTable->GetCityRank(scale, population, rank))
    return -1.0; // do not draw
  return rank;
}

void RulesHolder::ClearCaches()
{
  ForEachRule(bind(static_cast<void (BaseRule::*)()>(&BaseRule::MakeEmptyID), _4));
}

void RulesHolder::ResizeCaches(size_t s)
{
  ForEachRule(bind(&BaseRule::CheckCacheSize, _4, s));
}

RulesHolder & rules()
{
  static RulesHolder holder;
  return holder;
}

namespace
{
  namespace proto_rules
  {
    class Line : public BaseRule
    {
      LineDefProto m_line;
    public:
      Line(LineRuleProto const & r)
      {
        m_line.set_color(r.color());
        m_line.set_width(r.width());
        if (r.has_dashdot())
          *(m_line.mutable_dashdot()) = r.dashdot();
        if (r.has_pathsym())
          *(m_line.mutable_pathsym()) = r.pathsym();
        if (r.has_join())
          m_line.set_join(r.join());
        if (r.has_cap())
          m_line.set_cap(r.cap());
      }

      virtual LineDefProto const * GetLine() const { return &m_line; }
    };

    class Area : public BaseRule
    {
      AreaRuleProto m_area;
    public:
      Area(AreaRuleProto const & r) : m_area(r) {}

      virtual AreaRuleProto const * GetArea() const { return &m_area; }
    };

    class Symbol : public BaseRule
    {
      SymbolRuleProto m_symbol;
    public:
      Symbol(SymbolRuleProto const & r) : m_symbol(r)
      {
        if (r.has_apply_for_type())
          SetType(r.apply_for_type());
      }

      virtual SymbolRuleProto const * GetSymbol() const { return &m_symbol; }
    };

    template <class T> class CaptionT : public BaseRule
    {
      T m_caption;

    public:
      CaptionT(T const & r) : m_caption(r) {}

      virtual CaptionDefProto const * GetCaption(int i) const
      {
        if (i == 0)
          return &m_caption.primary();
        else
        {
          ASSERT_EQUAL ( i, 1, () );
          if (m_caption.has_secondary())
            return &m_caption.secondary();
          else
            return 0;
        }
      }
    };

    typedef CaptionT<CaptionRuleProto> Caption;
    typedef CaptionT<PathTextRuleProto> PathText;

    class Circle : public BaseRule
    {
      CircleRuleProto m_circle;
    public:
      Circle(CircleRuleProto const & r) : m_circle(r) {}

      virtual CircleRuleProto const * GetCircle() const { return &m_circle; }
    };

    class Shield : public BaseRule
    {
      ShieldRuleProto m_shield;
    public:
      Shield(ShieldRuleProto const & r) : m_shield(r) {}

      virtual ShieldRuleProto const * GetShield() const { return &m_shield; }
    };
  }

  class DoSetIndex
  {
  public:
    ContainerProto m_cont;

  private:
    vector<string> m_names;

    typedef ClassifElementProto ElementT;

    class RandI : public iterator_facade<
        RandI,
        ElementT const,
        random_access_traversal_tag>
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
      bool operator() (ElementT const & e1, ElementT const & e2) const
      {
        return (e1.name() < e2.name());
      }
      bool operator() (string const & e1, ElementT const & e2) const
      {
        return (e1 < e2.name());
      }
      bool operator() (ElementT const & e1, string const & e2) const
      {
        return (e1.name() < e2);
      }
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
    void AddRule(ClassifObject * p, int scale, rule_type_t type, TProtoRule const & rule,
                 vector<string> const & apply_if)
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
      obj->SetSelector(move(selector));
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
    DoSetIndex(RulesHolder & holder)
      : m_holder(holder) {}

    void operator() (ClassifObject * p)
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

          if (de.has_circle())
            AddRule<Circle>(p, de.scale(), circle, de.circle(), apply_if);

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
}

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
          if (rule.has_color())
          {
            bgColorDefault = rule.color();

            if (de.has_scale())
              bgColorForScale.insert(make_pair(de.scale(), rule.color()));
          }
        }
      }
      break;
    }
  }

  ASSERT_EQUAL(m_bgColors.size(), scales::UPPER_STYLE_SCALE+1, ());
  for (int scale = 0; scale <= scales::UPPER_STYLE_SCALE; ++scale)
  {
    auto const i = bgColorForScale.find(scale);
    if (bgColorForScale.end() != i)
      m_bgColors[scale] = i->second;
    else
      m_bgColors[scale] = bgColorDefault;
  }
}

void RulesHolder::LoadFromBinaryProto(string const & s)
{
  Clean();

  DoSetIndex doSet(*this);

  CHECK ( doSet.m_cont.ParseFromString(s), ("Error in proto loading!") );

  classif().GetMutableRoot()->ForEachObject(ref(doSet));

  InitBackgroundColors(doSet.m_cont);
}

void RulesHolder::LoadCityRankTableFromString(string & s)
{
  unique_ptr<ICityRankTable> table;

  if (!s.empty())
  {
    table = GetCityRankTableFromString(s);

    if (nullptr == table)
      LOG(LINFO, ("Invalid city-rank-table file"));
  }

  if (nullptr == table)
    table = GetConstRankCityRankTable();

  m_cityRankTable = move(table);
}

void LoadRules()
{
  string buffer;

  // Load drules_proto
  GetStyleReader().GetDrawingRulesReader().ReadAsString(buffer);
  rules().LoadFromBinaryProto(buffer);

  // Load city_rank
  buffer.clear();
  try
  {
    ReaderPtr<Reader> cityRankFileReader = GetPlatform().GetReader("city_rank.txt");
    cityRankFileReader.ReadAsString(buffer);
  }
  catch (FileAbsentException & e)
  {
    // city-rank.txt file is optional, if it does not exist, then default city-rank-table is used
    LOG(LINFO, ("File city-rank-table does not exist", e.Msg()));
  }
  rules().LoadCityRankTableFromString(buffer);
}

}
