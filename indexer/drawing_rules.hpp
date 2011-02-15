#pragma once
#include "drawing_rule_def.hpp"

#include "../base/base.hpp"

#include "../std/fstream.hpp"
#include "../std/map.hpp"
#include "../std/vector.hpp"
#include "../std/array.hpp"
#include "../std/string.hpp"

class FileReaderStream;
class FileWriterStream;

namespace drule
{
  typedef map<string, string> attrs_map_t;

  class BaseRule
  {
    string m_class;
    char m_type;

    mutable uint32_t m_id;

  public:
    static uint32_t const empty_id = 0xFFFFFFFF;

    BaseRule() : m_id(empty_id) {}

    virtual ~BaseRule() {}

    uint32_t GetID() const { return m_id; }
    void SetID(uint32_t id) const { m_id = id; }
    void MakeEmptyID() { m_id = empty_id; }

    void SetClassName(string const & cl) { m_class = cl; }
    void SetType(char type) { m_type = type; }

    char GetType() const { return m_type; }

    bool IsEqualBase(BaseRule const * p) const { return (m_type == p->m_type); }
    void ReadBase(FileReaderStream & ar);
    void WriteBase(FileWriterStream & ar) const;

    virtual bool IsEqual(BaseRule const * p) const = 0;
    virtual void Read(FileReaderStream & ar) = 0;
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

    void PushAttributes(string objClass, attrs_map_t & attrs);

    Key CreateRuleImpl1(string const & name, uint8_t type, string const & clValue, attrs_map_t const & attrs, bool isMask);
    Key CreateRuleImpl2(string const & name, uint8_t type, string const & clName, attrs_map_t const & attrs);

  public:
    ~RulesHolder();

    size_t AddRule(int32_t scale, rule_type_t type, BaseRule * p);
    size_t AddLineRule(int32_t scale, int color, double pixWidth);

    void Clean();

    void SetParseFile(char const * fPath, int scale);

    void CreateRules(string const & name, uint8_t type, attrs_map_t const & attrs, vector<Key> & v);

    BaseRule const * Find(Key const & k) const;

    int GetScale() const { return m_currScale; }

    void Read(FileReaderStream & s);
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
            toDo(i->first, j, m_container[j][v[k]]);
          }
        }
      }
    }
  };

  void WriteRules(char const * fPath);
  void ReadRules(char const * fPath);

  RulesHolder & rules();
}
