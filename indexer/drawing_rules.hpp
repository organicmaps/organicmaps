#pragma once
#include "drawing_rule_def.hpp"

#include "../base/base.hpp"
#include "../base/buffer_vector.hpp"

#include "../std/map.hpp"
#include "../std/vector.hpp"
#include "../std/array.hpp"
#include "../std/string.hpp"


class LineDefProto;
class AreaRuleProto;
class SymbolRuleProto;
class CaptionDefProto;
class CircleRuleProto;


namespace drule
{
  class BaseRule
  {
    mutable buffer_vector<uint32_t, 8> m_id1, m_id2;
    char m_type;      // obsolete for new styles, can be removed

  public:
    static uint32_t const empty_id = 0xFFFFFFFF;

    BaseRule() : m_type(node | way)
    {
    }
    virtual ~BaseRule() {}

    void CheckSize(buffer_vector<uint32_t, 8> & v, size_t s) const
    {
      if (v.size() < s)
        v.resize(s, empty_id);
    }

    uint32_t GetID(size_t threadID) const
    {
      CheckSize(m_id1, threadID + 1);
      return m_id1[threadID];
    }

    void SetID(size_t threadID, uint32_t id) const
    {
      CheckSize(m_id1, threadID + 1);
      m_id1[threadID] = id;
    }

    void MakeEmptyID(size_t threadID)
    {
      CheckSize(m_id1, threadID + 1);
      m_id1[threadID] = empty_id;
    }

    void MakeEmptyID()
    {
      for (size_t i = 0; i < m_id1.size(); ++i)
        MakeEmptyID(i);
    }

    uint32_t GetID2(size_t threadID) const
    {
      CheckSize(m_id2, threadID + 1);
      return m_id2[threadID];
    }

    void SetID2(size_t threadID, uint32_t id) const
    {
      CheckSize(m_id2, threadID + 1);
      m_id2[threadID] = id;
    }

    void MakeEmptyID2(size_t threadID)
    {
      CheckSize(m_id2, threadID + 1);
      m_id2[threadID] = empty_id;
    }

    void MakeEmptyID2()
    {
      for (size_t i = 0; i < m_id2.size(); ++i)
        MakeEmptyID2(i);
    }

    void SetType(char type) { m_type = type; }
    inline char GetType() const { return m_type; }

    virtual LineDefProto const * GetLine() const { return 0; }
    virtual AreaRuleProto const * GetArea() const { return 0; }
    virtual SymbolRuleProto const * GetSymbol() const { return 0; }
    virtual CaptionDefProto const * GetCaption(int) const { return 0; }
    virtual CircleRuleProto const * GetCircle() const { return 0; }
  };

  class RulesHolder
  {
    // container of rules by type
    typedef vector<BaseRule*> rule_vec_t;
    array<rule_vec_t, count_of_rules> m_container;

    /// scale -> array of rules by type -> index of rule in m_container
    typedef map<int32_t, array<vector<uint32_t>, count_of_rules> > rules_map_t;
    rules_map_t m_rules;

  public:
    ~RulesHolder();

    size_t AddRule(int scale, rule_type_t type, BaseRule * p);

    void Clean();

    void ClearCaches();

    BaseRule const * Find(Key const & k) const;

    void LoadFromTextProto(string const & buffer);

    template <class ToDo> void ForEachRule(ToDo toDo)
    {
      for (rules_map_t::const_iterator i = m_rules.begin(); i != m_rules.end(); ++i)
      {
        for (int j = 0; j < count_of_rules; ++j)
        {
          vector<uint32_t> const & v = i->second[j];
          for (size_t k = 0; k < v.size(); ++k)
          {
            // scale, type, rule
            toDo(i->first, j, v[k], m_container[j][v[k]]);
          }
        }
      }
    }
  };

  RulesHolder & rules();
}
