#include "../base/SRC_FIRST.hpp"

#include "drawing_rules.hpp"
#include "scales.hpp"
#include "classificator.hpp"

#ifdef OMIM_PRODUCTION
  #include "drules_struct_lite.pb.h"
#else
  #include "drules_struct.pb.h"
#endif

#include "../std/bind.hpp"
#include "../std/iterator_facade.hpp"

#include <google/protobuf/text_format.h>


namespace drule {

BaseRule::BaseRule() : m_type(node | way)
{}

BaseRule::~BaseRule()
{}

void BaseRule::CheckCacheSize(size_t s)
{
  m_id1.resize(s);
  MakeEmptyID();

  m_id2.resize(s);
  MakeEmptyID2();
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

uint32_t BaseRule::GetID2(size_t threadSlot) const
{
  ASSERT(m_id2.size() > threadSlot, ());
  return m_id2[threadSlot];
}

void BaseRule::SetID2(size_t threadSlot, uint32_t id) const
{
  ASSERT(m_id2.size() > threadSlot, ());
  m_id2[threadSlot] = id;
}

void BaseRule::MakeEmptyID2(size_t threadSlot)
{
  ASSERT(m_id2.size() > threadSlot, ());
  m_id2[threadSlot] = empty_id;
}

void BaseRule::MakeEmptyID2()
{
  for (size_t i = 0; i < m_id2.size(); ++i)
    MakeEmptyID2(i);
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

size_t RulesHolder::AddRule(int scale, rule_type_t type, BaseRule * p)
{
  ASSERT ( 0 <= scale && scale <= scales::GetUpperStyleScale(), (scale) );
  ASSERT ( 0 <= type && type < count_of_rules, () );

  m_container[type].push_back(p);

  vector<uint32_t> & v = m_rules[scale][type];
  v.push_back(m_container[type].size()-1);

  size_t const ret = v.size() - 1;
  ASSERT ( Find(Key(scale, type, ret)) == p, (ret) );
  return ret;
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

void RulesHolder::ClearCaches()
{
  ForEachRule(bind(&BaseRule::MakeEmptyID, _4));
  ForEachRule(bind(&BaseRule::MakeEmptyID2, _4));
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
    void AddRule(ClassifObject * p, int scale, rule_type_t type, TProtoRule const & rule)
    {
      size_t const i = m_holder.AddRule(scale, type, new TRule(rule));
      Key k(scale, type, i);

      p->SetVisibilityOnScale(true, scale);
      k.SetPriority(rule.priority());
      p->AddDrawRule(k);
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
        ClassifElementProto const & ce = m_cont.cont(i);
        for (int j = 0; j < ce.element_size(); ++j)
        {
          DrawElementProto const & de = ce.element(j);

          using namespace proto_rules;

          for (size_t k = 0; k < de.lines_size(); ++k)
            AddRule<Line>(p, de.scale(), line, de.lines(k));

          if (de.has_area())
            AddRule<Area>(p, de.scale(), area, de.area());

          if (de.has_symbol())
            AddRule<Symbol>(p, de.scale(), symbol, de.symbol());

          if (de.has_caption())
            AddRule<Caption>(p, de.scale(), caption, de.caption());

          if (de.has_circle())
            AddRule<Circle>(p, de.scale(), circle, de.circle());

          if (de.has_path_text())
            AddRule<PathText>(p, de.scale(), pathtext, de.path_text());
        }
      }

      p->ForEachObject(bind<void>(ref(*this), _1));

      m_names.pop_back();
    }
  };
}

#ifndef OMIM_PRODUCTION
void RulesHolder::LoadFromTextProto(string const & buffer)
{
  Clean();

  DoSetIndex doSet(*this);
  google::protobuf::TextFormat::ParseFromString(buffer, &doSet.m_cont);

  classif().GetMutableRoot()->ForEachObject(bind<void>(ref(doSet), _1));
}

void RulesHolder::SaveToBinaryProto(string const & buffer, ostream & s)
{
  ContainerProto cont;
  google::protobuf::TextFormat::ParseFromString(buffer, &cont);

  CHECK ( cont.SerializeToOstream(&s), ("Error in proto saving!") );
}
#endif

void RulesHolder::LoadFromBinaryProto(string const & s)
{
  Clean();

  DoSetIndex doSet(*this);

  CHECK ( doSet.m_cont.ParseFromString(s), ("Error in proto loading!") );

  classif().GetMutableRoot()->ForEachObject(bind<void>(ref(doSet), _1));
}

}
