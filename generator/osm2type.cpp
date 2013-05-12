#include "osm2type.hpp"
#include "xml_element.hpp"

#include "../indexer/classificator.hpp"
#include "../indexer/feature_visibility.hpp"

#include "../base/assert.hpp"
#include "../base/string_utils.hpp"
#include "../base/math.hpp"

#include "../std/vector.hpp"

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
      while (it)
      {
        if (strings::IsInArray(aTrue, *it)) return 1;
        if (strings::IsInArray(aFalse, *it)) return -1;
        ++it;
      }

      // "~" means no this tag, so sometimes it means true,
      // and all other cases - false. Choose according to key.
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

    template <class ToDo> class tags_wrapper
    {
      typedef typename ToDo::result_type res_t;

      string const & m_key;
      ToDo & m_toDo;
      res_t & m_res;

    public:
      tags_wrapper(string const & key, ToDo & toDo, res_t & res)
        : m_key(key), m_toDo(toDo), m_res(res) {}

      void operator() (string const & v)
      {
        if (!m_res)
          m_res = m_toDo(m_key, v);
      }
    };

    template <class ToDo>
    typename ToDo::result_type for_each_tag(XMLElement * p, ToDo toDo)
    {
      typedef typename ToDo::result_type res_t;

      res_t res = res_t();
      for (size_t i = 0; i < p->childs.size(); ++i)
      {
        if (p->childs[i].name == "tag")
        {
          string const & k = p->childs[i].attrs["k"];
          string const & v = p->childs[i].attrs["v"];

          if (k.empty() || is_skip_tag(k))
            continue;

          // this means "no"
          if (get_mark_value(k, v) == -1)
            continue;

          strings::Tokenize(v, ";", tags_wrapper<ToDo>(k, toDo, res));
          if (res) return res;
        }
      }
      return res;
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
      bool m_tunnel;

    public:
      typedef bool result_type;

      do_find_name(size_t & count, FeatureParams & params)
        : m_count(count), m_params(params), m_tunnel(false)
      {
        m_count = 0;
      }
      ~do_find_name()
      {
        if (m_tunnel && m_params.layer < 0)
          m_params.layer = feature::LAYER_TRANSPARENT_TUNNEL;
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

        if (v.empty()) return false;

        // get name with language suffix
        string lang;
        if (GetLangByKey(k, lang))
        {
          // Unicode Compatibility Decomposition,
          // followed by Canonical Composition (NFKC).
          // Needed for better search matching
          QByteArray const normBytes = QString::fromUtf8(
                v.c_str()).normalized(QString::NormalizationForm_KC).toUtf8();
          m_params.name.AddString(lang, normBytes.constData());
        }

        // get layer
        if (k == "layer" && m_params.layer == 0)
        {
          m_params.layer = atoi(v.c_str());
          int8_t const bound = 10;
          m_params.layer = my::clamp(m_params.layer, -bound, bound);
        }

        // get reference (we process road numbers only)
        if (k == "ref")
          m_params.ref = v;

        // get house number
        if (k == "addr:housenumber")
            m_params.AddHouseNumber(v);
        if (k == "addr:housename")
            m_params.AddHouseName(v);
        if (k == "addr:flats")
            m_params.flats = v;

        // get population rank
        if (k == "population")
        {
          uint64_t n;
          if (strings::to_uint64(v, n))
            m_params.rank = static_cast<uint8_t>(log(double(n)) / log(1.1));
        }

        // set 'tunnel' flag
        if (k == "tunnel")
          m_tunnel = true;

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

        uint64_t dummy;
        if (!m_isKey && strings::to_uint64(v, dummy))
          return (k == "admin_level");

        return true;
      }

    public:
      typedef ClassifObjectPtr result_type;

      do_find_obj(ClassifObject const * p, bool isKey) : m_parent(p), m_isKey(isKey) {}

      ClassifObjectPtr operator() (string const & k, string const & v) const
      {
        if (is_good_tag(k, v))
        {
          ClassifObjectPtr p = m_parent->BinaryFind(m_isKey ? k : v);
          if (p) return p;
        }
        return ClassifObjectPtr(0, 0);
      }
    };

    typedef vector<ClassifObjectPtr> path_type;

    class do_find_root_obj
    {
      set<string> const & m_skipTags;
      path_type & m_path;

    public:
      typedef ClassifObjectPtr result_type;

      do_find_root_obj(set<string> const & skipTags, path_type & path)
        : m_skipTags(skipTags), m_path(path)
      {
      }

      ClassifObjectPtr operator() (string const & k, string const & v) const
      {
        if (m_skipTags.find(k) == m_skipTags.end())
        {
          // first try to match key
          ClassifObjectPtr p = do_find_obj(classif().GetRoot(), true)(k, v);
          if (p)
          {
            m_path.push_back(p);

            // now try to match correspondent value
            p = do_find_obj(p.get(), false)(k, v);
            if (p) m_path.push_back(p);
          }
        }

        return (!m_path.empty() ? m_path.back() : ClassifObjectPtr(0, 0));
      }
    };
  }

  ClassifObjectPtr find_object(ClassifObject const * parent, XMLElement * p, bool isKey)
  {
    return for_each_tag(p, do_find_obj(parent, isKey));
  }

  size_t process_common_params(XMLElement * p, FeatureParams & params)
  {
    size_t count;
    for_each_tag(p, do_find_name(count, params));
    return count;
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

  uint32_t GetAddressType()
  {
    static char const * arr[] = { "building", "address" };
    static uint32_t const res = classif().GetTypeByPath(vector<string>(arr, arr + 2));
    return res;
  }

  void GetNameAndType(XMLElement * p, FeatureParams & params)
  {
//#ifdef DEBUG
//    // code to set a breakpoint
//    if (for_each_tag(p, debug_find_string("bridge")))
//    {
//      int break_here = 0;
//    }
//#endif

    // maybe an empty feature
    if (process_common_params(p, params) == 0)
      return;

    set<string> skipRootKeys;

    do
    {
      path_type path;

      // find first root object by key
      do_find_root_obj doFindRoot(skipRootKeys, path);
      (void)for_each_tag(p, doFindRoot);

      if (path.empty())
        break;

      // continue find path from last element
      do
      {
        // next objects trying to find by value first
        ClassifObjectPtr pObj = find_object(path.back().get(), p, false);
        if (!pObj)
        {
          // if no - try find object by key (in case of k = "area", v = "yes")
          pObj = find_object(path.back().get(), p, true);
        }

        // add to path or stop search
        if (pObj)
          path.push_back(pObj);
        else
          break;

      } while (true);

      // assign type
      uint32_t t = ftype::GetEmptyValue();
      for (size_t i = 0; i < path.size(); ++i)
        ftype::PushValue(t, path[i].GetIndex());

      // use features only with drawing rules
      if (feature::IsDrawableAny(t))
        params.AddType(t);

      // save this root to skip, and try again
      skipRootKeys.insert(path[0]->GetName());

    } while (true);

    if (!params.IsValid() && !params.house.IsEmpty())
    {
      params.name.Clear();
      // If we have address (house name or number), we should assign valid type.
      // There are a lot of features like this in Czech Republic.
      params.AddType(GetAddressType());
    }
  }

  uint32_t GetBoundaryType2()
  {
    char const * arr[] = { "boundary", "administrative" };
    return classif().GetTypeByPath(vector<string>(arr, arr + 2));
  }
}
