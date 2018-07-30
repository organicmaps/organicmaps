#pragma once

#include "indexer/drawing_rule_def.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/map_style.hpp"
#include "indexer/scales.hpp"
#include "indexer/types_mapping.hpp"

#include "base/macros.hpp"

#include <bitset>
#include <initializer_list>
#include <iostream>
#include <string>
#include <vector>

class ClassifObject;

namespace ftype
{
  inline uint32_t GetEmptyValue() { return 1; }

  void PushValue(uint32_t & type, uint8_t value);
  bool GetValue(uint32_t type, uint8_t level, uint8_t & value);
  void PopValue(uint32_t & type);
  void TruncValue(uint32_t & type, uint8_t level);
  uint8_t GetLevel(uint32_t type);
}

class ClassifObjectPtr
{
  ClassifObject const * m_p;
  size_t m_ind;

public:
  ClassifObjectPtr() : m_p(0), m_ind(0) {}
  ClassifObjectPtr(ClassifObject const * p, size_t i): m_p(p), m_ind(i) {}

  ClassifObject const * get() const { return m_p; }
  ClassifObject const * operator->() const { return m_p; }
  operator bool() const { return (m_p != 0); }

  size_t GetIndex() const { return m_ind; }
};

class ClassifObject
{
  struct less_name_t
  {
    bool operator() (ClassifObject const & r1, ClassifObject const & r2) const
    {
      return (r1.m_name < r2.m_name);
    }
  };

public:
  ClassifObject() {}  // for serialization only
  ClassifObject(std::string const & s) : m_name(s) {}

  /// @name Fill from osm draw rule files.
  //@{
private:
  ClassifObject * AddImpl(std::string const & s);
public:
  ClassifObject * Add(std::string const & s);
  ClassifObject * Find(std::string const & s);

  void AddDrawRule(drule::Key const & k);
  //@}

  /// @name Find substitution when reading osm features.
  //@{
  ClassifObjectPtr BinaryFind(std::string const & s) const;
  //@}

  void Sort();
  void Swap(ClassifObject & r);

  std::string const & GetName() const { return m_name; }
  ClassifObject const * GetObject(size_t i) const;

  void ConcatChildNames(std::string & s) const;

  void GetSuitable(int scale, feature::EGeomType ft, drule::KeysT & keys) const;
  inline std::vector<drule::Key> const & GetDrawingRules() const { return m_drawRule; }

  bool IsDrawable(int scale) const;
  bool IsDrawableAny() const;
  bool IsDrawableLike(feature::EGeomType ft, bool emptyName = false) const;

  pair<int, int> GetDrawScaleRange() const;

  template <class ToDo>
  void ForEachObject(ToDo toDo)
  {
    for (size_t i = 0; i < m_objs.size(); ++i)
      toDo(&m_objs[i]);
  }

  template <class ToDo>
  void ForEachObjectInTree(ToDo & toDo, uint32_t const start) const
  {
    for (size_t i = 0; i < m_objs.size(); ++i)
    {
      uint32_t type = start;

      ftype::PushValue(type, static_cast<uint8_t>(i));

      toDo(&m_objs[i], type);

      m_objs[i].ForEachObjectInTree(toDo, type);
    }
  }

  using VisibleMask = std::bitset<scales::UPPER_STYLE_SCALE+1>;
  void SetVisibilityOnScale(bool isVisible, int scale) { m_visibility[scale] = isVisible; }

  /// @name Policies for classificator tree serialization.
  //@{
  class BasePolicy
  {
  protected:
    std::vector<ClassifObject *> m_stack;
    ClassifObject * Current() const { return m_stack.back(); }

  public:
    BasePolicy(ClassifObject * pRoot) { m_stack.push_back(pRoot); }

    void Start(size_t i) { m_stack.push_back(&(Current()->m_objs[i])); }
    void End() { m_stack.pop_back(); }
  };

  class LoadPolicy : public BasePolicy
  {
    typedef BasePolicy base_type;
  public:
    LoadPolicy(ClassifObject * pRoot) : base_type(pRoot) {}

    void Name(std::string const & name) { Current()->m_name = name; }
    void Start(size_t i);
    void EndChilds();
  };
  //@}

private:
  std::string m_name;
  std::vector<drule::Key> m_drawRule;
  std::vector<ClassifObject> m_objs;
  VisibleMask m_visibility;
};

inline void swap(ClassifObject & r1, ClassifObject & r2)
{
  r1.Swap(r2);
}

class Classificator
{
  DISALLOW_COPY_AND_MOVE(Classificator);

  ClassifObject m_root;

  IndexAndTypeMapping m_mapping;

  uint32_t m_coastType;

  static ClassifObject * AddV(ClassifObject * parent, std::string const & key, std::string const & value);

public:
  Classificator() : m_root("world") {}

  ClassifObject * Add(ClassifObject * parent, std::string const & key, std::string const & value);

  /// @name Serialization-like functions.
  //@{
  void ReadClassificator(std::istream & s);
  void ReadTypesMapping(std::istream & s);

  void SortClassificator();
  //@}

  void Clear();

  bool HasTypesMapping() const { return m_mapping.IsLoaded(); }

  /// Return type by path in classificator tree, for example
  /// path = ["natural", "caostline"].
  //@{
private:
  template <class IterT> uint32_t GetTypeByPathImpl(IterT beg, IterT end) const;
public:
  /// @return 0 in case of nonexisting type
  uint32_t GetTypeByPathSafe(std::vector<std::string> const & path) const;
  /// Invokes ASSERT in case of nonexisting type
  uint32_t GetTypeByPath(std::vector<std::string> const & path) const;
  uint32_t GetTypeByPath(std::initializer_list<char const *> const & lst) const;
  /// @see GetReadableObjectName().
  /// @returns 0 in case of nonexisting type.
  uint32_t GetTypeByReadableObjectName(std::string const & name) const;
  //@}

  uint32_t GetIndexForType(uint32_t t) const { return m_mapping.GetIndex(t); }
  // Throws std::out_of_range exception.
  uint32_t GetTypeForIndex(uint32_t i) const { return m_mapping.GetType(i); }
  bool IsTypeValid(uint32_t t) const { return m_mapping.HasIndex(t); }

  inline uint32_t GetCoastType() const { return m_coastType; }

  /// @name used in osm2type.cpp, not for public use.
  //@{
  ClassifObject const * GetRoot() const { return &m_root; }
  ClassifObject * GetMutableRoot() { return &m_root; }
  //@}

  /// Iterate through all classificator tree.
  /// Functor receives pointer to object and uint32 type.
  template <class ToDo> void ForEachTree(ToDo & toDo) const
  {
    GetRoot()->ForEachObjectInTree(toDo, ftype::GetEmptyValue());
  }

  /// @name Used only in feature_visibility.cpp, not for public use.
  //@{
  template <class ToDo> typename ToDo::ResultType
  ProcessObjects(uint32_t type, ToDo & toDo) const;

  ClassifObject const * GetObject(uint32_t type) const;
  std::string GetFullObjectName(uint32_t type) const;
  //@}

  /// @return Object name to show in UI (not for debug purposes).
  std::string GetReadableObjectName(uint32_t type) const;
};

Classificator & classif();
