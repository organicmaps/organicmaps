#pragma once

#include "indexer/drawing_rule_def.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/scales.hpp"
#include "indexer/types_mapping.hpp"

#include "base/macros.hpp"
#include "base/stl_helpers.hpp"

#include <bitset>
#include <string>
#include <utility>
#include <vector>

class ClassifObject;

namespace ftype
{
  inline uint32_t GetEmptyValue() { return 1; }

  void PushValue(uint32_t & type, uint8_t value);
  /// @pre level < GetLevel(type).
  uint8_t GetValue(uint32_t type, uint8_t level);
  void PopValue(uint32_t & type);
  void TruncValue(uint32_t & type, uint8_t level);
  inline uint32_t Trunc(uint32_t type, uint8_t level)
  {
    TruncValue(type, level);
    return type;
  }
  uint8_t GetLevel(uint32_t type);
}

class ClassifObjectPtr
{
  ClassifObject const * m_p;
  size_t m_ind;

public:
  ClassifObjectPtr() : m_p(nullptr), m_ind(0) {}
  ClassifObjectPtr(ClassifObject const * p, size_t i): m_p(p), m_ind(i) {}

  ClassifObject const * get() const { return m_p; }
  ClassifObject const * operator->() const { return m_p; }
  explicit operator bool() const { return (m_p != nullptr); }

  size_t GetIndex() const { return m_ind; }
};

class ClassifObject
{
  struct LessName
  {
    bool operator() (ClassifObject const & r1, ClassifObject const & r2) const
    {
      return (r1.m_name < r2.m_name);
    }
    bool operator() (ClassifObject const & r1, std::string_view r2) const
    {
      return (r1.m_name < r2);
    }
    bool operator() (std::string_view r1, ClassifObject const & r2) const
    {
      return (r1 < r2.m_name);
    }
  };

public:
  ClassifObject() = default;  // for serialization only
  explicit ClassifObject(std::string s) : m_name(std::move(s)) {}

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
  ClassifObjectPtr BinaryFind(std::string_view s) const;
  //@}

  void Sort();
  void Swap(ClassifObject & r);

  std::string const & GetName() const { return m_name; }
  ClassifObject const * GetObject(size_t i) const;

  std::vector<drule::Key> const & GetDrawRules() const { return m_drawRules; }
  void GetSuitable(int scale, feature::GeomType gt, drule::KeysT & keys) const;

  // Returns std::numeric_limits<int>::min() if there are no overlay drules.
  int GetMaxOverlaysPriority() const { return m_maxOverlaysPriority; }

  bool IsDrawable(int scale) const;
  bool IsDrawableAny() const;
  bool IsDrawableLike(feature::GeomType gt, bool emptyName = false) const;

  std::pair<int, int> GetDrawScaleRange() const;

  /// @name Iterate first level children only.
  /// @{
  template <class ToDo> void ForEachObject(ToDo && toDo)
  {
    for (auto & e: m_objs)
      toDo(&e);
  }
  template <class ToDo> void ForEachObject(ToDo && toDo) const
  {
    for (auto const & e: m_objs)
      toDo(e);
  }
  /// @}

  // Recursive subtree iteration.
  template <typename ToDo>
  void ForEachObjectInTree(ToDo && toDo, uint32_t const start) const
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
  void SetVisibilityOnScale(bool isVisible, int scale) { m_visibility.set(scale, isVisible); }

  /// @name Policies for classificator tree serialization.
  //@{
  class BasePolicy
  {
  protected:
    std::vector<ClassifObject *> m_stack;
    ClassifObject * Current() const { return m_stack.back(); }

  public:
    explicit BasePolicy(ClassifObject * pRoot) { m_stack.push_back(pRoot); }
    // No polymorphism here.
    void Start(size_t i) { m_stack.push_back(&(Current()->m_objs[i])); }
    void End() { m_stack.pop_back(); }
  };

  class LoadPolicy : public BasePolicy
  {
    typedef BasePolicy base_type;
  public:
    explicit LoadPolicy(ClassifObject * pRoot) : base_type(pRoot) {}

    void Name(std::string const & name) { Current()->m_name = name; }
    void Start(size_t i);
    void EndChilds();
  };
  //@}

private:
  std::string m_name;
  std::vector<drule::Key> m_drawRules;
  std::vector<ClassifObject> m_objs;
  VisibleMask m_visibility;

  int m_maxOverlaysPriority = std::numeric_limits<int>::min();
};

inline void swap(ClassifObject & r1, ClassifObject & r2)
{
  r1.Swap(r2);
}

class Classificator
{
public:
  Classificator() : m_root("world") {}

  /// @name Serialization-like functions.
  //@{
  void ReadClassificator(std::istream & s);
  void ReadTypesMapping(std::istream & s);
  //@}

  void Clear();

  bool HasTypesMapping() const { return m_mapping.IsLoaded(); }

  static constexpr uint32_t INVALID_TYPE = IndexAndTypeMapping::INVALID_TYPE;

  /// @name Type by \a path in classificator tree, for example {"natural", "caostline"}.
  ///@{
  /// @return INVALID_TYPE in case of nonexisting type
  uint32_t GetTypeByPathSafe(std::vector<std::string_view> const & path) const;
  /// Invokes ASSERT in case of nonexisting type
  uint32_t GetTypeByPath(std::vector<std::string> const & path) const;
  uint32_t GetTypeByPath(std::vector<std::string_view> const & path) const;
  uint32_t GetTypeByPath(base::StringIL const & lst) const;
  ///@}

  /// @see GetReadableObjectName().
  /// @returns INVALID_TYPE in case of nonexisting type.
  uint32_t GetTypeByReadableObjectName(std::string const & name) const;

  uint32_t GetIndexForType(uint32_t t) const { return m_mapping.GetIndex(t); }

  /// @return INVALID_TYPE if \a ind is out of bounds.
  uint32_t GetTypeForIndex(uint32_t i) const { return m_mapping.GetType(i); }
  bool IsTypeValid(uint32_t t) const { return m_mapping.HasIndex(t); }

  inline uint32_t GetCoastType() const { return m_coastType; }
  inline uint32_t GetStubType() const { return m_stubType; }

  /// @name used in osm2type.cpp, not for public use.
  //@{
  ClassifObject const * GetRoot() const { return &m_root; }
  ClassifObject * GetMutableRoot() { return &m_root; }
  //@}

  /// Iterate through all classificator tree.
  /// Functor receives pointer to object and uint32 type.
  template <typename ToDo>
  void ForEachTree(ToDo && toDo) const
  {
    GetRoot()->ForEachObjectInTree(std::forward<ToDo>(toDo), ftype::GetEmptyValue());
  }

  template <typename ToDo>
  void ForEachInSubtree(ToDo && toDo, uint32_t root) const
  {
    toDo(root);
    GetObject(root)->ForEachObjectInTree([&toDo](ClassifObject const *, uint32_t c) { toDo(c); },
                                         root);
  }

  ClassifObject const * GetObject(uint32_t type) const;
  std::string GetFullObjectName(uint32_t type) const;
  std::vector<std::string> GetFullObjectNamePath(uint32_t type) const;

  /// @return Object name to show in UI (not for debug purposes).
  std::string GetReadableObjectName(uint32_t type) const;

private:
  template <class ToDo> void ForEachPathObject(uint32_t type, ToDo && toDo) const;

  template <typename Iter>
  uint32_t GetTypeByPathImpl(Iter beg, Iter end) const;

  ClassifObject m_root;
  IndexAndTypeMapping m_mapping;
  uint32_t m_coastType = 0;
  uint32_t m_stubType = 0;

  DISALLOW_COPY_AND_MOVE(Classificator);
};

Classificator & classif();
