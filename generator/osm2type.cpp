#include "generator/osm2type.hpp"
#include "generator/osm2meta.hpp"
#include "generator/xml_element.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_visibility.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"
#include "base/math.hpp"

#include "std/vector.hpp"
#include "std/bind.hpp"
#include "std/function.hpp"
#include "std/initializer_list.hpp"

#include <QtCore/QString>


namespace ftype
{
  namespace
  {
    /// get value of mark (1 == "yes", -1 == "no", 0 == not a "yes\no")
    static int get_mark_value(string const & k, string const & v)
    {
      static char const * aTrue[] = { "yes", "true", "1", "*" };
      static char const * aFalse[] = { "no", "false", "-1" };

      strings::SimpleTokenizer it(v, "|");
      if (it)
      {
        bool allowedKey = (k != "layer" && k != "oneway");
        while (it)
        {
          string const &part = *it;
          if (allowedKey && strings::IsInArray(aFalse, part))
            return -1;

          if (strings::IsInArray(aTrue, part))
            return 1;

          ++it;
        }
      }

      // "~" means no this tag, so sometimes it means true,
      // and all other cases - false. Choose according to a key.
      if (v == "~")
        return (k == "access" ? 1 : -1);

      return 0;
    }

    bool is_skip_tag(string const & k)
    {
      return (k == "created_by"
           || k == "description"
           || k == "cycleway"     // [highway=primary][cycleway=lane] parsed as [highway=cycleway]
           || k == "proposed"     // [highway=proposed][proposed=primary] parsed as [highway=primary]
           || k == "construction" // [highway=primary][construction=primary] parsed as [highway=construction]
           );
    }

    template <typename TResult, class ToDo>
    TResult for_each_tag(XMLElement * p, ToDo toDo)
    {
      TResult res = TResult();
      for (auto & e : p->m_tags)
      {
        if (e.key.empty() || is_skip_tag(e.key))
          continue;

        // this means "no"
        if (get_mark_value(e.key, e.value) == -1)
          continue;

        res = toDo(e.key, e.value);
        if (res)
          return res;
      }
      return res;
    }

    template <typename TResult, class ToDo>
    TResult for_each_tag_ex(XMLElement * p, ToDo toDo, set<int> & skipTags)
    {
      int id = 0;
      return for_each_tag<TResult>(p, [&](string const & k, string const & v)
      {
        TResult res = TResult();
        if (skipTags.count(id) == 0)
        {
          res = toDo(k, v);
          if (res)
          {
            skipTags.insert(id);
            return res;
          }
        }
        ++id;
        return res;
      });
    }

    bool is_name_tag(string const & k)
    {
      return (string::npos != k.find("name"));
    }

    class do_print
    {
      ostream & m_s;
    public:
      typedef bool result_type;

      do_print(ostream & s) : m_s(s) {}
      bool operator() (string const & k, string const & v) const
      {
        m_s << k << " <---> " << v << endl;
        return false;
      }
    };

    class do_find_name
    {
      set<string> m_savedNames;

      size_t & m_count;
      FeatureParams & m_params;

    public:
      do_find_name(size_t & count, FeatureParams & params)
        : m_count(count), m_params(params)
      {
        m_count = 0;
      }

      bool GetLangByKey(string const & k, string & lang)
      {
        strings::SimpleTokenizer token(k, "\t :");
        if (!token)
          return false;

        // this is an international (latin) name
        if (*token == "int_name")
          lang = "int_name";
        else
        {
          if (*token == "name")
          {
            ++token;
            lang = (token ? *token : "default");

            // replace dummy arabian tag with correct tag
            if (lang == "ar1")
              lang = "ar";
          }
        }

        if (lang.empty())
          return false;

        // avoid duplicating names
        return m_savedNames.insert(lang).second;
      }

      bool operator() (string const & k, string const & v)
      {
        ++m_count;

        if (v.empty())
          return false;

        // get name with language suffix
        string lang;
        if (GetLangByKey(k, lang))
        {
          // Unicode Compatibility Decomposition,
          // followed by Canonical Composition (NFKC).
          // Needed for better search matching
          QByteArray const normBytes = QString::fromUtf8(
                v.c_str()).normalized(QString::NormalizationForm_KC).toUtf8();
          m_params.AddName(lang, normBytes.constData());
        }

        // get layer
        if (k == "layer" && m_params.layer == 0)
        {
          m_params.layer = atoi(v.c_str());
          int8_t const bound = 10;
          m_params.layer = my::clamp(m_params.layer, -bound, bound);
        }

        // get reference (we process road numbers only)
        if (k == "ref" && v != "route")
          m_params.ref = v;

        // get house number
        if (k == "addr:housenumber")
        {
          // Treat "numbers" like names if it's not an actual number.
          if (!m_params.AddHouseNumber(v))
            m_params.AddHouseName(v);
        }
        if (k == "addr:housename")
          m_params.AddHouseName(v);
        if (k == "addr:street")
          m_params.AddStreetAddress(v);
        if (k == "addr:flats")
          m_params.flats = v;

        // get population rank
        if (k == "population")
        {
          uint64_t n;
          if (strings::to_uint64(v, n))
            m_params.rank = static_cast<uint8_t>(log(double(n)) / log(1.1));
        }

        return false;
      }
    };

    class do_find_obj
    {
      ClassifObject const * m_parent;
      bool m_isKey;

      bool is_good_tag(string const & k, string const & v) const
      {
        if (is_name_tag(k))
          return false;

        if (!m_isKey)
        {
          // Take numbers only for "capital" and "admin_level" now.
          // NOTE! If you add a new type into classificator, which has a number in it
          // (like admin_level=1 or capital=2), please don't forget to insert it here too.
          // Otherwise generated data will not contain your newly added features.

          if (strings::is_number(v))
            return (k == "admin_level" || k == "capital");
        }
        return true;
      }

    public:
      do_find_obj(ClassifObject const * p, bool isKey) : m_parent(p), m_isKey(isKey) {}

      ClassifObjectPtr operator() (string const & k, string const & v) const
      {
        return (is_good_tag(k, v) ? m_parent->BinaryFind(m_isKey ? k : v) : ClassifObjectPtr(0, 0));
      }
    };

    typedef vector<ClassifObjectPtr> path_type;

    class do_find_key_value_obj
    {
      ClassifObject const * m_parent;
      path_type & m_path;

    public:
      do_find_key_value_obj(ClassifObject const * p, path_type & path)
        : m_parent(p), m_path(path)
      {
      }

      bool operator() (string const & k, string const & v)
      {
        // first try to match key
        ClassifObjectPtr p = do_find_obj(m_parent, true)(k, v);
        if (p)
        {
          m_path.push_back(p);

          // now try to match correspondent value
          p = do_find_obj(p.get(), false)(k, v);
          if (p)
            m_path.push_back(p);
          return true;
        }

        return false;
      }
    };

    class TagProcessor
    {
      template <typename FuncT>
      struct Rule
      {
        char const * key;
        // * - take any values
        // ! - take only negative values
        // ~ - take only positive values
        char const * value;
        function<FuncT> func;
      };

      static bool IsNegative(string const & value)
      {
        for (char const * s : { "no", "none", "false" })
          if (value == s)
            return true;
        return false;
      }

      XMLElement * m_element;

    public:
      TagProcessor(XMLElement * elem) : m_element(elem) {}

      template <typename FuncT = void()>
      void ApplyRules(initializer_list<Rule<FuncT>> const & rules) const
      {
        for (auto & e : m_element->m_tags)
        {
          for (auto const & rule: rules)
          {
            if (e.key == rule.key)
            {
              bool take = false;
              if (rule.value[0] == '*')
                take = true;
              else if (rule.value[0] == '!')
                take = IsNegative(e.value);
              else if (rule.value[0] == '~')
                take = !IsNegative(e.value);

              if (take || e.value == rule.value)
                call(rule.func, e.key, e.value);
            }
          }
        }
      }

    protected:
      static void call(function<void()> const & f, string &, string &) { f(); }
      static void call(function<void(string &, string &)> const & f, string & k, string & v) { f(k, v); }
    };
  }

  size_t ProcessCommonParams(XMLElement * p, FeatureParams & params)
  {
    size_t count;
    for_each_tag<bool>(p, do_find_name(count, params));
    return count;
  }

  void AddLayers(XMLElement * p)
  {
    bool hasLayer = false;
    char const * layer = nullptr;

    TagProcessor(p).ApplyRules({
      { "bridge", "yes", [&layer]() { layer = "1"; }},
      { "tunnel", "yes", [&layer]() { layer = "-1"; }},
      { "layer", "*", [&hasLayer]() { hasLayer = true; }},
    });

    if (!hasLayer && layer)
      p->AddTag("layer", layer);
  }

//#ifdef DEBUG
//  class debug_find_string
//  {
//    string m_comp;
//  public:
//    debug_find_string(string const & comp) : m_comp(comp) {}
//    typedef bool result_type;
//    bool operator() (string const & k, string const & v) const
//    {
//      return (k == m_comp || v == m_comp);
//    }
//  };
//#endif

  class CachedTypes
  {
    buffer_vector<uint32_t, 16> m_types;

  public:
    enum EType { ENTRANCE, HIGHWAY, ADDRESS, ONEWAY, PRIVATE, LIT, NOFOOT, YESFOOT };

    CachedTypes()
    {
      Classificator const & c = classif();
      
      for (auto const & e : (StringIL[]) { {"entrance"}, {"highway"} })
        m_types.push_back(c.GetTypeByPath(e));

      StringIL arr[] =
      {
        {"building", "address"}, {"hwtag", "oneway"}, {"hwtag", "private"},
        {"hwtag", "lit"}, {"hwtag", "nofoot"}, {"hwtag", "yesfoot"}
      };
      for (auto const & e : arr)
        m_types.push_back(c.GetTypeByPath(e));
    }

    uint32_t Get(EType t) const { return m_types[t]; }
    bool IsHighway(uint32_t t) const
    {
      ftype::TruncValue(t, 1);
      return t == Get(HIGHWAY);
    }
  };

  void GetNameAndType(XMLElement * p, FeatureParams & params)
  {
    /// Process synonym tags to match existing classificator types.
    /// @todo We are planning to rewrite classificator <-> tags matching.
    TagProcessor(p).ApplyRules<void(string &, string &)>(
    {
      { "atm", "yes", [](string &k, string &v) { k.swap(v); k = "amenity"; }},
      { "restaurant", "yes", [](string &k, string &v) { k.swap(v); k = "amenity"; }},
      { "hotel", "yes", [](string &k, string &v) { k.swap(v); k = "tourism"; }},
    });

    AddLayers(p);

    // maybe an empty feature
    if (ProcessCommonParams(p, params) == 0)
      return;

    set<int> skipRows;
    ClassifObject const * root = classif().GetRoot();
    do
    {
      path_type path;

      // find first root object by key
      if (!for_each_tag_ex<bool>(p, do_find_key_value_obj(root, path), skipRows))
        break;
      CHECK(!path.empty(), ());

      do
      {
        // continue find path from last element
        ClassifObject const * parent = path.back().get();

        // next objects trying to find by value first
        ClassifObjectPtr pObj = for_each_tag_ex<ClassifObjectPtr>(p, do_find_obj(parent, false), skipRows);

        if (pObj)
        {
          path.push_back(pObj);
        }
        else
        {
          // if no - try find object by key (in case of k = "area", v = "yes")
          if (!for_each_tag_ex<bool>(p, do_find_key_value_obj(parent, path), skipRows))
            break;
        }
      } while (true);

      // assign type
      uint32_t t = ftype::GetEmptyValue();
      for (auto const & e : path)
        ftype::PushValue(t, e.GetIndex());

      // use features only with drawing rules
      if (feature::IsDrawableAny(t))
        params.AddType(t);

    } while (true);

    static CachedTypes const types;

    if (!params.house.IsEmpty())
    {
      // Delete "entrance" type for house number (use it only with refs).
      // Add "address" type if we have house number but no valid types.
      if (params.PopExactType(types.Get(CachedTypes::ENTRANCE)))
      {
        params.name.Clear();
        // If we have address (house name or number), we should assign valid type.
        // There are a lot of features like this in Czech Republic.
        params.AddType(types.Get(CachedTypes::ADDRESS));
      }
    }

    for (size_t i = 0; i < params.m_Types.size(); ++i)
      if (types.IsHighway(params.m_Types[i]))
      {
        TagProcessor(p).ApplyRules(
        {
          { "oneway", "yes", [&params]() { params.AddType(types.Get(CachedTypes::ONEWAY)); }},
          { "oneway", "1", [&params]() { params.AddType(types.Get(CachedTypes::ONEWAY)); }},
          { "oneway", "-1", [&params]() { params.AddType(types.Get(CachedTypes::ONEWAY)); params.m_reverseGeometry = true; }},
          { "access", "private", [&params]() { params.AddType(types.Get(CachedTypes::PRIVATE)); }},
          { "lit", "~", [&params]() { params.AddType(types.Get(CachedTypes::LIT)); }},

          { "foot", "!", [&params]() { params.AddType(types.Get(CachedTypes::NOFOOT)); }},

          { "foot", "~", [&params]() { params.AddType(types.Get(CachedTypes::YESFOOT)); }},
          { "sidewalk", "~", [&params]() { params.AddType(types.Get(CachedTypes::YESFOOT)); }},
        });
        break;
      }

    params.FinishAddingTypes();

    // Collect addidtional information about feature such as
    // hotel stars, opening hours, cuisine, ...
    for_each_tag<bool>(p, MetadataTagProcessor(params));
  }

  bool IsValidTypes(FeatureParams const & params)
  {
    // Add final types checks (during parsing and generation process) here.
    return params.IsValid();
  }
}
