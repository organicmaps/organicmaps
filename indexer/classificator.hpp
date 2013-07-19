#pragma once
#include "drawing_rule_def.hpp"
#include "types_mapping.hpp"
#include "scales.hpp"

#include "../base/base.hpp"

#include "../std/vector.hpp"
#include "../std/string.hpp"
#include "../std/iostream.hpp"
#include "../std/bitset.hpp"


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
  ClassifObject(string const & s) : m_name(s) {}

  /// @name Fill from osm draw rule files.
  //@{
private:
  ClassifObject * AddImpl(string const & s);
public:
  ClassifObject * Add(string const & s);
  void AddCriterion(string const & s);
  ClassifObject * Find(string const & s);

  void AddDrawRule(drule::Key const & k);
  //@}

  /// @name Find substitution when reading osm features.
  //@{
  ClassifObjectPtr BinaryFind(string const & s) const;
  //@}

  void Clear() { m_objs.clear(); }

  void Sort();
  void Swap(ClassifObject & r);

  bool IsCriterion() const;
  string const & GetName() const { return m_name; }
  ClassifObject const * GetObject(size_t i) const;

  void ConcatChildNames(string & s) const;

  enum FeatureGeoType { FEATURE_TYPE_POINT = 0, FEATURE_TYPE_LINE, FEATURE_TYPE_AREA };
  void GetSuitable(int scale, FeatureGeoType ft, drule::KeysT & keys) const;
  inline vector<drule::Key> const & GetDrawingRules() const { return m_drawRule; }

  bool IsDrawable(int scale) const;
  bool IsDrawableAny() const;
  bool IsDrawableLike(FeatureGeoType ft) const;

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

  typedef bitset<scales::UPPER_STYLE_SCALE+1> visible_mask_t;
  visible_mask_t GetVisibilityMask() const { return m_visibility; }
  void SetVisibilityMask(visible_mask_t mask) { m_visibility = mask; }
  void SetVisibilityOnScale(const bool isVisible, const int scale) { m_visibility[scale] = isVisible; }

  /// @name Policies for classificator tree serialization.
  //@{
  class BasePolicy
  {
  protected:
    vector<ClassifObject *> m_stack;
    ClassifObject * Current() const { return m_stack.back(); }

  public:
    BasePolicy(ClassifObject * pRoot) { m_stack.push_back(pRoot); }

    void Start(size_t i) { m_stack.push_back(&(Current()->m_objs[i])); }
    void End() { m_stack.pop_back(); }
  };

  class SavePolicy : public BasePolicy
  {
  public:
    SavePolicy(ClassifObject * pRoot) : BasePolicy(pRoot) {}

    string Name() const { return Current()->m_name; }
    void Serialize(ostream & s) const;

    size_t BeginChilds() const { return Current()->m_objs.size(); }
  };

  class LoadPolicy : public BasePolicy
  {
    typedef BasePolicy base_type;
  public:
    LoadPolicy(ClassifObject * pRoot) : base_type(pRoot) {}

    void Name(string const & name) { Current()->m_name = name; }
    void Serialize(string const & s);

    void Start(size_t i);
    void EndChilds();
  };

  class VisSavePolicy : public SavePolicy
  {
  public:
    VisSavePolicy(ClassifObject * pRoot) : SavePolicy(pRoot) {}

    void Serialize(ostream & s) const;
  };

  class VisLoadPolicy : public BasePolicy
  {
    typedef BasePolicy base_type;

  public:
    VisLoadPolicy(ClassifObject * pRoot) : BasePolicy(pRoot) {}

    void Name(string const & name) const;
    void Serialize(string const & s);

    void Start(size_t i);
    void EndChilds() {}
  };
  //@}

private:
  string m_name;
  vector<drule::Key> m_drawRule;
  vector<ClassifObject> m_objs;
  visible_mask_t m_visibility;

  typedef vector<ClassifObject>::iterator iter_t;
  typedef vector<ClassifObject>::const_iterator const_iter_t;
};

inline void swap(ClassifObject & r1, ClassifObject & r2)
{
  r1.Swap(r2);
}

class Classificator
{
  ClassifObject m_root;

  IndexAndTypeMapping m_mapping;

  uint32_t m_coastType;

  static ClassifObject * AddV(ClassifObject * parent, string const & key, string const & value);

public:
  Classificator() : m_root("world") {}

  ClassifObject * Add(ClassifObject * parent, string const & key, string const & value);

  /// @name Serialization-like functions.
  //@{
  void ReadClassificator(istream & s);
  void PrintClassificator(char const * fPath);

  void ReadTypesMapping(istream & s);

  void SortClassificator();
  //@}

  void Clear();

  /// Return type by path in classificator tree, example:
  /// path = ["natural", "caostline"].
  uint32_t GetTypeByPath(vector<string> const & path) const;

  uint32_t GetIndexForType(uint32_t t) const { return m_mapping.GetIndex(t); }
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
  string GetFullObjectName(uint32_t type) const;
  //@}

  /// @return Object name to show in UI (not for debug purposes).
  string GetReadableObjectName(uint32_t type) const;
};

Classificator & classif();
