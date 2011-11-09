#pragma once
#include "drawing_rule_def.hpp"

#include "../base/base.hpp"

#include "../std/map.hpp"
#include "../std/vector.hpp"
#include "../std/array.hpp"
#include "../std/string.hpp"
#include "../base/buffer_vector.hpp"

class ReaderPtrStream;
class FileWriterStream;

namespace drule
{
  typedef map<string, string> AttrsMapType;

  class BaseRule
  {
    string m_class;
    mutable buffer_vector<uint32_t, 8> m_id1, m_id2;
    char m_type;

  public:
    static uint32_t const empty_id = 0xFFFFFFFF;

    BaseRule()
    {
    }

    virtual ~BaseRule() {}

    /// @todo Rewrite this. Make an array of IDs.
    //@{
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
    //@}

    void SetClassName(string const & cl) { m_class = cl; }
    void SetType(char type) { m_type = type; }

    char GetType() const { return m_type; }

    bool IsEqualBase(BaseRule const * p) const { return (m_type == p->m_type); }
    void ReadBase(ReaderPtrStream & ar);
    void WriteBase(FileWriterStream & ar) const;

    virtual bool IsEqual(BaseRule const * p) const = 0;
    virtual void Read(ReaderPtrStream & ar) = 0;
    virtual void Write(FileWriterStream & ar) const = 0;

    /// @name This functions can tell us about the type of rule.
    //@{
    virtual int GetColor() const { return -1; }           ///< path "line" color
    virtual int GetFillColor()const { return -1; }        ///< fill "area" color
    virtual double GetTextHeight() const { return -1.0; } ///< text height of "caption"
    //@}

    virtual unsigned char GetAlpha () const { return 255; }
    virtual double GetWidth() const { return -1; }
    virtual void GetPattern(vector<double> &, double &) const {}
    virtual void GetSymbol(string &) const {}

    virtual double GetRadius() const { return -1; }
  };

  class RulesHolder
  {
    // container of rules by type
    typedef vector<BaseRule*> rule_vec_t;
    array<rule_vec_t, count_of_rules> m_container;

    /// scale -> array of rules by type -> index of rule in m_container
    typedef map<int32_t, array<vector<uint32_t>, count_of_rules> > rules_map_t;
    rules_map_t m_rules;

    /// @name temporary for search rules parameters by 'class' attribute
    //@{
    string m_file;
    int m_currScale;
    //@}

    void PushAttributes(string objClass, AttrsMapType & attrs);

    Key CreateRuleImpl1(string const & name, uint8_t type, string const & clValue,
                        AttrsMapType const & attrs, bool isMask);
    Key CreateRuleImpl2(string const & name, uint8_t type, string const & clName,
                        AttrsMapType const & attrs);

  public:
    ~RulesHolder();

    size_t AddRule(int scale, rule_type_t type, BaseRule * p);
    size_t AddLineRule(int scale, int color, double pixWidth);
    size_t AddAreaRule(int scale, int color);
    size_t AddSymbolRule(int scale, string const & sym);

    void Clean();

    void SetParseFile(char const * fPath, int scale);

    void CreateRules(string const & name, uint8_t type,
                     AttrsMapType const & attrs, vector<Key> & v);

    BaseRule const * Find(Key const & k) const;

    int GetScale() const { return m_currScale; }

    void Read(ReaderPtrStream & s);
    void Write(FileWriterStream & s);

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

  void WriteRules(string const & fPath);
  void ReadRules(ReaderPtrStream & s);

  RulesHolder & rules();
}
