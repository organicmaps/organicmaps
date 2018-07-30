#pragma once

#include "indexer/drawing_rule_def.hpp"
#include "indexer/drules_selector.hpp"
#include "indexer/map_style.hpp"

#include "base/base.hpp"
#include "base/buffer_vector.hpp"

#include "std/target_os.hpp"

#include <array>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

class LineDefProto;
class AreaRuleProto;
class SymbolRuleProto;
class CaptionDefProto;
class ShieldRuleProto;
class ContainerProto;
class FeatureType;


namespace drule
{
  class BaseRule
  {
    mutable buffer_vector<uint32_t, 4> m_id1;
    char m_type;      // obsolete for new styles, can be removed

    std::unique_ptr<ISelector> m_selector;

  public:
    static uint32_t const empty_id = 0xFFFFFFFF;

    BaseRule();
    virtual ~BaseRule();

    void CheckCacheSize(size_t s);

    uint32_t GetID(size_t threadSlot) const;
    void SetID(size_t threadSlot, uint32_t id) const;

    void MakeEmptyID(size_t threadSlot);
    void MakeEmptyID();

    void SetType(char type) { m_type = type; }
    inline char GetType() const { return m_type; }

    virtual LineDefProto const * GetLine() const;
    virtual AreaRuleProto const * GetArea() const;
    virtual SymbolRuleProto const * GetSymbol() const;
    virtual CaptionDefProto const * GetCaption(int) const;
    virtual text_type_t GetCaptionTextType(int) const;
    virtual ShieldRuleProto const * GetShield() const;

    // Test feature by runtime feature style selector
    // Returns true if rule is applicable for feature, otherwise it returns false
    bool TestFeature(FeatureType & ft, int zoom) const;

    // Set runtime feature style selector
    void SetSelector(unique_ptr<ISelector> && selector);
  };

  class RulesHolder
  {
    // container of rules by type
    using RuleVec = std::vector<BaseRule*>;
    std::array<RuleVec, count_of_rules> m_container;

    /// scale -> array of rules by type -> index of rule in m_container
    using RulesMap = std::map<int32_t, std::array<std::vector<uint32_t>, count_of_rules>>;
    RulesMap m_rules;

    /// background color for scales in range [0...scales::UPPER_STYLE_SCALE]
    std::vector<uint32_t> m_bgColors;

    std::unordered_map<std::string, uint32_t> m_colors;

  public:
    RulesHolder();
    ~RulesHolder();

    Key AddRule(int scale, rule_type_t type, BaseRule * p);

    void Clean();

    void ClearCaches();
    void ResizeCaches(size_t Size);

    BaseRule const * Find(Key const & k) const;

    uint32_t GetBgColor(int scale) const;
    uint32_t GetColor(std::string const & name) const;

#ifdef OMIM_OS_DESKTOP
    void LoadFromTextProto(std::string const & buffer);
    static void SaveToBinaryProto(std::string const & buffer, std::ostream & s);
#endif

    void LoadFromBinaryProto(std::string const & s);

    template <class ToDo> void ForEachRule(ToDo toDo)
    {
      for (RulesMap::const_iterator i = m_rules.begin(); i != m_rules.end(); ++i)
      {
        for (int j = 0; j < count_of_rules; ++j)
        {
          std::vector<uint32_t> const & v = i->second[j];
          for (size_t k = 0; k < v.size(); ++k)
          {
            // scale, type, rule
            toDo(i->first, j, v[k], m_container[j][v[k]]);
          }
        }
      }
    }

  private:
    void InitBackgroundColors(ContainerProto const & cp);
    void InitColors(ContainerProto const & cp);
  };

  RulesHolder & rules();

  void LoadRules();
}
