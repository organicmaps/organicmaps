#include "map/framework.hpp"

#include "search/result.hpp"
#include "search/reverse_geocoder.hpp"

#include "drape_frontend/visual_params.hpp"

#include "indexer/categories_holder.hpp"
#include "indexer/classificator.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_visibility.hpp"

#include "platform/preferred_languages.hpp"

/*
namespace
{
  class FeatureInfoT
  {
  public:
    FeatureInfoT(double d, feature::TypesHolder & types,
                 string & name, string & house, m2::PointD const & pt)
      : m_types(types), m_pt(pt), m_dist(d)
    {
      m_name.swap(name);
      m_house.swap(house);
    }

    bool operator<(FeatureInfoT const & rhs) const
    {
      return (m_dist < rhs.m_dist);
    }

    void Swap(FeatureInfoT & rhs)
    {
      swap(m_dist, rhs.m_dist);
      swap(m_pt, rhs.m_pt);
      m_name.swap(rhs.m_name);
      m_house.swap(rhs.m_house);
      swap(m_types, rhs.m_types);
    }

    string m_name, m_house;
    feature::TypesHolder m_types;
    m2::PointD m_pt;
    double m_dist;
  };

  void swap(FeatureInfoT & i1, FeatureInfoT & i2)
  {
    i1.Swap(i2);
  }

//  string DebugPrint(FeatureInfoT const & info)
//  {
//    return ("Name = " + info.m_name +
//            " House = " + info.m_house +
//            " Distance = " + strings::to_string(info.m_dist));
//  }

  class DoGetFeatureInfoBase
  {
  protected:
    virtual bool IsInclude(double dist, feature::TypesHolder const & types) const = 0;
    virtual double GetResultDistance(double d, feature::TypesHolder const & types) const = 0;
    virtual double NeedProcess(feature::TypesHolder const & types) const
    {
      // feature should be visible in needed scale
      pair<int, int> const r = feature::GetDrawableScaleRange(types);
      return base::between_s(r.first, r.second, m_scale);
    }

    /// @return epsilon value for distance compare according to priority:
    /// point feature is better than linear, that is better than area.
    static double GetCompareEpsilonImpl(feature::EGeomType type, double eps)
    {
      using namespace feature;
      switch (type)
      {
      case GEOM_POINT: return 0.0 * eps;
      case GEOM_LINE: return 1.0 * eps;
      case GEOM_AREA: return 2.0 * eps;
      default:
        ASSERT ( false, () );
        return numeric_limits<double>::max();
      }
    }

  public:
    DoGetFeatureInfoBase(m2::PointD const & pt, int scale)
      : m_pt(pt), m_scale(scale)
    {
      m_coastType = classif().GetCoastType();
    }

    void operator() (FeatureType const & f)
    {
      feature::TypesHolder types(f);
      if (!types.Has(m_coastType) && NeedProcess(types))
      {
        // Convert from meters to degrees for backward compatibility.
        double const d = feature::GetMinDistanceMeters(f, m_pt, m_scale) * MercatorBounds::degreeInMeters;
        ASSERT_GREATER_OR_EQUAL(d, 0.0, ());

        if (IsInclude(d, types))
        {
          string name;
          f.GetReadableName(name);
          string house = f.GetHouseNumber();

          // if geom type is not GEOM_POINT, result center point doesn't matter in future use
          m2::PointD const pt =
              (types.GetGeoType() == feature::GEOM_POINT) ? f.GetCenter() : m2::PointD();

          // name, house are assigned like move semantics
          m_cont.push_back(FeatureInfoT(GetResultDistance(d, types), types, name, house, pt));
        }
      }
    }

    void SortResults()
    {
      sort(m_cont.begin(), m_cont.end());
    }

  private:
    m2::PointD m_pt;
    uint32_t m_coastType;

  protected:
    int m_scale;
    vector<FeatureInfoT> m_cont;
  };

  class DoGetFeatureTypes : public DoGetFeatureInfoBase
  {
  protected:
    virtual bool IsInclude(double dist, feature::TypesHolder const & types) const
    {
      return (dist <= m_eps);
    }
    virtual double GetResultDistance(double d, feature::TypesHolder const & types) const
    {
      return (d + GetCompareEpsilonImpl(types.GetGeoType(), m_eps));
    }

  public:
    DoGetFeatureTypes(m2::PointD const & pt, double eps, int scale)
      : DoGetFeatureInfoBase(pt, scale), m_eps(eps)
    {
    }

    void GetFeatureTypes(size_t count, vector<string> & types)
    {
      SortResults();

      Classificator const & c = classif();

      for (size_t i = 0; i < min(count, m_cont.size()); ++i)
        for (uint32_t t : m_cont[i].m_types)
          types.push_back(c.GetReadableObjectName(t));
    }

  private:
    double m_eps;
  };
}

void Framework::GetFeatureTypes(m2::PointD const & pxPoint, vector<string> & types) const
{
  m2::AnyRectD rect;
  m_currentModelView.GetTouchRect(pxPoint, df::VisualParams::Instance().GetTouchRectRadius(), rect);

  // This scale should fit in geometry scales range.
  int const scale = min(GetDrawScale(), scales::GetUpperScale());

  DoGetFeatureTypes getTypes(rect.GlobalCenter(), rect.GetMaxSize() / 2.0, scale);
  m_model.ForEachFeature(rect.GetGlobalRect(), getTypes, scale);

  getTypes.GetFeatureTypes(5, types);
}

namespace
{
  class DoGetAddressBase : public DoGetFeatureInfoBase
  {
  public:
    class TypeChecker
    {
      vector<uint32_t> m_localities, m_streets, m_buildings;
      int m_localityScale;

      template <size_t count, size_t ind>
      void FillMatch(char const * (& arr)[count][ind], vector<uint32_t> & vec)
      {
        static_assert(count > 0, "");
        static_assert(ind > 0, "");

        Classificator const & c = classif();

        vec.reserve(count);
        for (size_t i = 0; i < count; ++i)
        {
          vector<string> v(arr[i], arr[i] + ind);
          vec.push_back(c.GetTypeByPath(v));
        }
      }

      static bool IsMatchImpl(vector<uint32_t> const & vec, feature::TypesHolder const & types)
      {
        for (uint32_t t : types)
        {
          ftype::TruncValue(t, 2);
          if (find(vec.begin(), vec.end(), t) != vec.end())
            return true;
        }

        return false;
      }

    public:
      TypeChecker()
      {
        char const * arrLocalities[][2] = {
          { "place", "city" },
          { "place", "town" },
          { "place", "village" },
          { "place", "hamlet" }
        };

        char const * arrStreet[][2] = {
          { "highway", "primary" },
          { "highway", "secondary" },
          { "highway", "residential" },
          { "highway", "tertiary" },
          { "highway", "living_street" },
          { "highway", "service" }
        };

        char const * arrBuilding[][1] = {
          { "building" }
        };

        FillMatch(arrLocalities, m_localities);
        m_localityScale = 0;
        for (size_t i = 0; i < m_localities.size(); ++i)
        {
          m_localityScale = max(m_localityScale,
                                feature::GetDrawableScaleRange(m_localities[i]).first);
        }

        FillMatch(arrStreet, m_streets);
        FillMatch(arrBuilding, m_buildings);
      }

      int GetLocalitySearchScale() const { return m_localityScale; }

      bool IsLocality(feature::TypesHolder const & types) const
      {
        return IsMatchImpl(m_localities, types);
      }
      bool IsStreet(feature::TypesHolder const & types) const
      {
        return IsMatchImpl(m_streets, types);
      }

      bool IsBuilding(feature::TypesHolder const & types) const
      {
        return IsMatchImpl(m_buildings, types);
      }

      double GetLocalityDivideFactor(feature::TypesHolder const & types) const
      {
        double arrF[] = { 10.0, 10.0, 1.0, 1.0 };
        ASSERT_EQUAL ( ARRAY_SIZE(arrF), m_localities.size(), () );

        for (uint32_t t : types)
        {
          ftype::TruncValue(t, 2);

          auto j = find(m_localities.begin(), m_localities.end(), t);
          if (j != m_localities.end())
            return arrF[distance(m_localities.begin(), j)];
        }

        return 1.0;
      }
    };

  protected:
    TypeChecker const & m_checker;

  public:
    DoGetAddressBase(m2::PointD const & pt, int scale, TypeChecker const & checker)
      : DoGetFeatureInfoBase(pt, scale), m_checker(checker)
    {
    }
  };

  class DoGetAddressInfo : public DoGetAddressBase
  {
  protected:
    virtual bool IsInclude(double dist, feature::TypesHolder const & types) const
    {
      // 0 - point, 1 - linear, 2 - area;
      return (dist <= m_arrEps[types.GetGeoType()]);
    }

    virtual double GetResultDistance(double d, feature::TypesHolder const & types) const
    {
      return (d + GetCompareEpsilonImpl(types.GetGeoType(), 5.0 * MercatorBounds::degreeInMeters));
    }

    virtual double NeedProcess(feature::TypesHolder const & types) const
    {
      using namespace feature;

      if (m_scale > -1)
      {
        // we need features with texts for address lookup
        pair<int, int> const r = GetDrawableScaleRangeForRules(types, RULE_ANY_TEXT | RULE_SYMBOL);
        return base::between_s(r.first, r.second, m_scale);
      }
      else
        return true;
    }

    static void GetReadableTypes(search::Engine const * eng, int8_t locale,
                                 feature::TypesHolder & types,
                                 search::AddressInfo & info)
    {
      types.SortBySpec();

      // Try to add types from categories.
      for (uint32_t t : types)
      {
        string s;
        if (eng->GetNameByType(t, locale, s))
          info.m_types.push_back(s);
      }

      // If nothing added - return raw classificator types.
      if (info.m_types.empty())
      {
        Classificator const & c = classif();
        for (uint32_t t : types)
          info.m_types.push_back(c.GetReadableObjectName(t));
      }
    }

  public:
    DoGetAddressInfo(m2::PointD const & pt, int scale, TypeChecker const & checker,
                     double const (&arrRadius) [3])
      : DoGetAddressBase(pt, scale, checker)
    {
      for (size_t i = 0; i < 3; ++i)
      {
        // use average value to convert meters to degrees
        m2::RectD const r = MercatorBounds::RectByCenterXYAndSizeInMeters(pt, arrRadius[i]);
        m_arrEps[i] = (r.SizeX() + r.SizeY()) / 2.0;
      }
    }

    void FillAddress(search::Engine const * eng, search::AddressInfo & info)
    {
      int8_t const locale = CategoriesHolder::MapLocaleToInteger(languages::GetCurrentOrig());

      SortResults();

      for (size_t i = 0; i < m_cont.size(); ++i)
      {
        /// @todo Make logic better.
        /// Now we skip linear objects to get only POI's here (don't mix with streets or roads).
        /// But there are linear types that may be interesting for POI (rivers).
        if (m_cont[i].m_types.GetGeoType() != feature::GEOM_LINE)
        {
          info.m_name = m_cont[i].m_name;

          GetReadableTypes(eng, locale, m_cont[i].m_types, info);

          if (!info.m_name.empty())
            break;
        }
      }
    }

  private:
    double m_arrEps[3];
  };

  class DoGetLocality : public DoGetAddressBase
  {
  protected:
    virtual bool IsInclude(double dist, feature::TypesHolder const & types) const
    {
      return (dist <= m_eps);
    }

    virtual double GetResultDistance(double d, feature::TypesHolder const & types) const
    {
      // This routine is needed for quality of locality prediction.
      // Hamlet may be the nearest point, but it's a part of a City. So use the divide factor
      // for distance, according to feature type.
      return (d / m_checker.GetLocalityDivideFactor(types));
    }

    virtual double NeedProcess(feature::TypesHolder const & types) const
    {
      return (types.GetGeoType() == feature::GEOM_POINT && m_checker.IsLocality(types));
    }

  public:
    DoGetLocality(m2::PointD const & pt, int scale, TypeChecker const & checker,
                  m2::RectD const & rect)
      : DoGetAddressBase(pt, scale, checker)
    {
      // use maximum value to convert meters to degrees
      m_eps = max(rect.SizeX(), rect.SizeY());
    }

    void FillLocality(search::AddressInfo & info, Framework const & fm)
    {
      SortResults();
      //LOG(LDEBUG, (m_cont));

      for (size_t i = 0; i < m_cont.size(); ++i)
      {
        if (!m_cont[i].m_name.empty() && fm.GetCountryName(m_cont[i].m_pt) == info.m_country)
        {
          info.m_city = m_cont[i].m_name;
          break;
        }
      }
    }

  private:
    double m_eps;
  };
}

namespace
{
  /// Global instance for type checker.
  /// @todo Possible need to add synhronization.
  typedef DoGetAddressBase::TypeChecker CheckerT;
  CheckerT * g_checker = 0;

  CheckerT & GetChecker()
  {
    if (g_checker == 0)
      g_checker = new CheckerT();
    return *g_checker;
  }
}
*/

search::AddressInfo Framework::GetAddressInfoAtPoint(m2::PointD const & pt) const
{
  double const kDistanceThresholdMeters = 0.5;

  search::AddressInfo info;

  search::ReverseGeocoder const coder(m_model.GetDataSource());
  search::ReverseGeocoder::Address addr;
  coder.GetNearbyAddress(pt, addr);

  // We do not init nearby address info for points that are located
  // outside of the nearby building.
  if (addr.GetDistance() < kDistanceThresholdMeters)
  {
    info.m_house = addr.GetHouseNumber();
    info.m_street = addr.GetStreetName();
    info.m_distanceMeters = addr.GetDistance();
  }

  return info;
}

search::AddressInfo Framework::GetFeatureAddressInfo(FeatureID const & fid) const
{
  FeatureType ft;
  if (!GetFeatureByID(fid, ft))
    return {};
  return GetFeatureAddressInfo(ft);
}

search::AddressInfo Framework::GetFeatureAddressInfo(FeatureType & ft) const
{
  search::AddressInfo info;
  // @TODO(vng): insert correct implementation from new search.
  //info.m_country = GetCountryName(feature::GetCenter(ft));
  // @TODO(vng): Temporarily commented - it's slow and not used in UI.
  //GetLocality(pt, info);

  search::ReverseGeocoder const coder(m_model.GetDataSource());
  search::ReverseGeocoder::Address addr;
  if (coder.GetExactAddress(ft, addr))
  {
    info.m_house = addr.GetHouseNumber();
    info.m_street = addr.GetStreetName();
  }

  // TODO(vng): Why AddressInfo is responsible for types and names? Refactor out.
  string defaultName, intName;
  ft.GetPreferredNames(defaultName, intName);
  info.m_name = defaultName.empty() ? intName : defaultName;
  info.m_types = GetPrintableFeatureTypes(ft);

  return info;
}

vector<string> Framework::GetPrintableFeatureTypes(FeatureType & ft) const
{
  vector<string> results;

  feature::TypesHolder types(ft);
  types.SortBySpec();
  auto const & c = classif();
  for (uint32_t type : types)
    results.push_back(c.GetReadableObjectName(type));
  return results;
}

/*
void Framework::GetLocality(m2::PointD const & pt, search::AddressInfo & info) const
{
  CheckerT & checker = GetChecker();

  int const scale = checker.GetLocalitySearchScale();
  LOG(LDEBUG, ("Locality scale = ", scale));

  // radius to search localities
  m2::RectD const rect = MercatorBounds::RectByCenterXYAndSizeInMeters(pt, 20000.0);
  DoGetLocality getLocality(pt, scale, checker, rect);

  m_model.ForEachFeature(rect, getLocality, scale);

  getLocality.FillLocality(info, *this);
}
*/
