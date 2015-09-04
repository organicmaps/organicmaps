#include "generator/osm2type.hpp"
#include "generator/osm2meta.hpp"
#include "generator/osm_element.hpp"

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
    bool IgnoreTag(string const & k, string const & v)
    {
      static string const negativeValues[] = { "no", "false", "-1" };
      static pair<string const, bool const> const processedKeys[] = {
        {"description", true}
        ,{"cycleway", true}     // [highway=primary][cycleway=lane] parsed as [highway=cycleway]
        ,{"proposed", true}     // [highway=proposed][proposed=primary] parsed as [highway=primary]
        ,{"construction", true} // [highway=primary][construction=primary] parsed as [highway=construction]
        ,{"layer", false} // process in any case
        ,{"oneway", false} // process in any case
      };

      // ignore empty key
      if (k.empty())
        return true;

      // ignore some keys
      for (auto const & key : processedKeys)
        if (k == key.first)
          return key.second;

      // ignore keys with negative values
      for (auto const & value : negativeValues)
        if (v == value)
          return true;

      return false;
    }

    template <typename TResult, class ToDo>
    TResult for_each_tag(OsmElement * p, ToDo toDo)
    {
      TResult res = TResult();
      for (auto & e : p->m_tags)
      {
        if (IgnoreTag(e.key, e.value))
          continue;

        res = toDo(e.key, e.value);
        if (res)
          return res;
      }
      return res;
    }

    template <typename TResult, class ToDo>
    TResult for_each_tag_ex(OsmElement * p, ToDo toDo, set<int> & skipTags)
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

    class ExtractNames
    {
      set<string> m_savedNames;
      FeatureParams & m_params;

    public:
      ExtractNames(FeatureParams & params) : m_params(params) {}

      bool GetLangByKey(string const & k, string & lang)
      {
        strings::SimpleTokenizer token(k, "\t :");
        if (!token)
          return false;

        // this is an international (latin) name
        if (*token == "int_name")
          return m_savedNames.insert(lang = "int_name").second;

        if (*token != "name")
          return false;

        ++token;
        lang = (token ? *token : "default");

        // replace dummy arabian tag with correct tag
        if (lang == "ar1")
          lang = "ar";

        // avoid duplicating names
        return m_savedNames.insert(lang).second;
      }

      bool operator() (string & k, string & v)
      {
        string lang;
        if (v.empty() || !GetLangByKey(k, lang))
          return false;

        // Unicode Compatibility Decomposition,
        // followed by Canonical Composition (NFKC).
        // Needed for better search matching
        QByteArray const normBytes = QString::fromUtf8(
              v.c_str()).normalized(QString::NormalizationForm_KC).toUtf8();
        m_params.AddName(lang, normBytes.constData());
        k.clear();
        v.clear();
        return false;
      }
    };

    class do_find_obj
    {
      ClassifObject const * m_parent;
      bool m_isKey;

      bool is_good_tag(string const & k, string const & v) const
      {
        if (string::npos != k.find("name"))
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

      OsmElement * m_element;

    public:
      TagProcessor(OsmElement * elem) : m_element(elem) {}

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

  class CachedTypes
  {
    buffer_vector<uint32_t, 16> m_types;

  public:
    enum EType { ENTRANCE, HIGHWAY, ADDRESS, ONEWAY, PRIVATE, LIT, NOFOOT, YESFOOT,
                 RW_STATION, RW_STATION_SUBWAY };

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

      m_types.push_back(c.GetTypeByPath({ "railway", "station" }));
      m_types.push_back(c.GetTypeByPath({ "railway", "station", "subway" }));
    }

    uint32_t Get(EType t) const { return m_types[t]; }

    bool IsHighway(uint32_t t) const
    {
      ftype::TruncValue(t, 1);
      return t == Get(HIGHWAY);
    }
    bool IsRwStation(uint32_t t) const
    {
      return t == Get(RW_STATION);
    }
    bool IsRwSubway(uint32_t t) const
    {
      ftype::TruncValue(t, 3);
      return t == Get(RW_STATION_SUBWAY);
    }
  };

  void GetNameAndType(OsmElement * p, FeatureParams & params)
  {
//    std::cout << "--[" << p->Tags().size() << "]" << "--------------------------------------\n";
//    std::cout << DebugPrint(*p) << std::endl;
//    std::cout << "--\n";

    bool hasLayer = false;
    char const * layer = nullptr;

    TagProcessor(p).ApplyRules
    ({
      { "bridge", "yes", [&layer]() { layer = "1"; }},
      { "tunnel", "yes", [&layer]() { layer = "-1"; }},
      { "layer", "*", [&hasLayer]() { hasLayer = true; }},
    });

    if (!hasLayer && layer)
      p->AddTag("layer", layer);

    for_each_tag<bool>(p, ExtractNames(params));

    // Base rules for tags processing
    TagProcessor(p).ApplyRules<void(string &, string &)>
    ({
      { "atm", "yes", [](string &k, string &v) { k.swap(v); k = "amenity"; }},
      { "restaurant", "yes", [](string &k, string &v) { k.swap(v); k = "amenity"; }},
      { "hotel", "yes", [](string &k, string &v) { k.swap(v); k = "tourism"; }},
      { "addr:housename", "*", [&params](string &k, string &v) { params.AddHouseName(v); k.clear(); v.clear();}},
      { "addr:street", "*", [&params](string &k, string &v) { params.AddStreetAddress(v); k.clear(); v.clear();}},
      { "addr:flats", "*", [&params](string &k, string &v) { params.flats = v; k.clear(); v.clear();}},
      { "addr:housenumber", "*", [&params](string &k, string &v)
        {
          // Treat "numbers" like names if it's not an actual number.
          if (!params.AddHouseNumber(v))
            params.AddHouseName(v);
          k.clear(); v.clear();
        }},
      { "population", "*", [&params](string &k, string &v)
        {
          // get population rank
          uint64_t n;
          if (strings::to_uint64(v, n))
            params.rank = static_cast<uint8_t>(log(double(n)) / log(1.1));
          k.clear(); v.clear();
        }},
      { "ref", "*", [&params](string &k, string &v)
        {
          // get reference (we process road numbers only)
          params.ref = v;
          k.clear(); v.clear();
        }},
      { "layer", "*", [&params](string &k, string &v)
        {
          // get layer
          if (params.layer == 0)
          {
            params.layer = atoi(v.c_str());
            int8_t const bound = 10;
            params.layer = my::clamp(params.layer, -bound, bound);
          }
        }},
    });

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

    bool highwayDone = false;
    bool subwayDone = false;
    bool railwayDone = false;

    // Get a copy of source types, because we will modify params in the loop;
    FeatureParams::TTypes const vTypes = params.m_Types;
    for (size_t i = 0; i < vTypes.size(); ++i)
    {
      if (!highwayDone && types.IsHighway(vTypes[i]))
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

        highwayDone = true;
      }

      if (!subwayDone && types.IsRwSubway(vTypes[i]))
      {
        TagProcessor(p).ApplyRules(
        {
          { "network", "London Underground", [&params]() { params.SetRwSubwayType("london"); }},
          { "network", "New York City Subway", [&params]() { params.SetRwSubwayType("newyork"); }},
          { "network", "Московский метрополитен", [&params]() { params.SetRwSubwayType("moscow"); }},
          { "network", "Петербургский метрополитен", [&params]() { params.SetRwSubwayType("spb"); }},
          { "network", "Verkehrsverbund Berlin-Brandenburg", [&params]() { params.SetRwSubwayType("berlin"); }},
          { "network", "Минский метрополитен", [&params]() { params.SetRwSubwayType("minsk"); }},

          { "network", "Київський метрополітен", [&params]() { params.SetRwSubwayType("kiev"); }},
          { "operator", "КП «Київський метрополітен»", [&params]() { params.SetRwSubwayType("kiev"); }},

          { "network", "RATP", [&params]() { params.SetRwSubwayType("paris"); }},
          { "network", "Metro de Barcelona", [&params]() { params.SetRwSubwayType("barcelona"); }},

          { "network", "Metro de Madrid", [&params]() { params.SetRwSubwayType("madrid"); }},
          { "operator", "Metro de Madrid", [&params]() { params.SetRwSubwayType("madrid"); }},

          { "network", "Metropolitana di Roma", [&params]() { params.SetRwSubwayType("roma"); }},
          { "network", "ATAC", [&params]() { params.SetRwSubwayType("roma"); }},
        });

        subwayDone = true;
      }

      if (!subwayDone && !railwayDone && types.IsRwStation(vTypes[i]))
      {
        TagProcessor(p).ApplyRules(
        {
          { "network", "London Underground", [&params]() { params.SetRwSubwayType("london"); }},
        });

        railwayDone = true;
      }
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
