#include "framework.hpp"

#include "../indexer/classificator.hpp"
#include "../indexer/feature_visibility.hpp"


namespace
{
  class FeatureInfoT
  {
    double m_dist;

  public:
    FeatureInfoT(double d, feature::TypesHolder & types, string & name, m2::PointD const & pt)
      : m_dist(d), m_types(types), m_pt(pt)
    {
      m_name.swap(name);
    }

    bool operator<(FeatureInfoT const & rhs) const
    {
      return (m_dist < rhs.m_dist);
    }

    void Swap(FeatureInfoT & rhs)
    {
      swap(m_dist, rhs.m_dist);
      m_name.swap(rhs.m_name);
      swap(m_types, rhs.m_types);
    }

    string m_name;
    feature::TypesHolder m_types;
    m2::PointD m_pt;
  };

  void swap(FeatureInfoT & i1, FeatureInfoT & i2)
  {
    i1.Swap(i2);
  }

  class DoGetFeatureInfoBase
  {
  protected:
    virtual double GetResultDistance(double d, feature::TypesHolder const & types) const = 0;
    virtual double NeedProcess(feature::TypesHolder const & types) const
    {
      pair<int, int> const r = feature::GetDrawableScaleRange(types);
      return my::between_s(r.first, r.second, m_scale);
    }

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
    DoGetFeatureInfoBase(m2::PointD const & pt, double eps, int scale)
      : m_pt(pt), m_scale(scale), m_eps(eps)
    {
      m_coastType = classif().GetCoastType();
    }

    void operator() (FeatureType const & f)
    {
      feature::TypesHolder types(f);
      if (!types.Has(m_coastType) && NeedProcess(types))
      {
        double const d = f.GetDistance(m_pt, m_scale);
        ASSERT_GREATER_OR_EQUAL(d, 0.0, ());

        if (d <= m_eps)
        {
          string defName, intName;
          f.GetPreferredDrawableNames(defName, intName);

          // if geom type is not GEOM_POINT, result center point doesn't matter in future use
          m2::PointD const pt = (types.GetGeoType() == feature::GEOM_POINT) ?
                f.GetCenter() : m2::PointD();

          m_cont.push_back(FeatureInfoT(GetResultDistance(d, types), types, defName, pt));
        }
      }
    }

    void SortResults()
    {
      sort(m_cont.begin(), m_cont.end());
    }

  private:
    m2::PointD m_pt;
    int m_scale;
    uint32_t m_coastType;

  protected:
    double m_eps;
    vector<FeatureInfoT> m_cont;
  };

  class DoGetFeatureTypes : public DoGetFeatureInfoBase
  {
  protected:
    virtual double GetResultDistance(double d, feature::TypesHolder const & types) const
    {
      return (d + GetCompareEpsilonImpl(types.GetGeoType(), m_eps));
    }

  public:
    DoGetFeatureTypes(m2::PointD const & pt, double eps, int scale)
      : DoGetFeatureInfoBase(pt, eps, scale)
    {
    }

    void GetFeatureTypes(size_t count, vector<string> & types)
    {
      SortResults();

      Classificator const & c = classif();

      for (size_t i = 0; i < min(count, m_cont.size()); ++i)
        for (size_t j = 0; j < m_cont[i].m_types.Size(); ++j)
          types.push_back(c.GetFullObjectName(m_cont[i].m_types[j]));
    }
  };
}

void Framework::GetFeatureTypes(m2::PointD pt, vector<string> & types) const
{
  pt = m_navigator.ShiftPoint(pt);

  int const sm = 20;
  m2::RectD pixR(m2::PointD(pt.x - sm, pt.y - sm), m2::PointD(pt.x + sm, pt.y + sm));

  m2::RectD glbR;
  m_navigator.Screen().PtoG(pixR, glbR);

  int const scale = GetDrawScale();
  DoGetFeatureTypes getTypes(m_navigator.Screen().PtoG(pt),
                             max(glbR.SizeX(), glbR.SizeY()) / 2.0,
                             scale);

  m_model.ForEachFeature(glbR, getTypes, scale);

  getTypes.GetFeatureTypes(5, types);
}


namespace
{
  class DoGetAddressInfo : public DoGetFeatureInfoBase
  {
  public:
    class TypeChecker
    {
      vector<uint32_t> m_localities, m_streets, m_buildings;

      template <size_t count, size_t ind>
      void FillMatch(char const * (& arr)[count][ind], vector<uint32_t> & vec)
      {
        STATIC_ASSERT ( count > 0 );
        STATIC_ASSERT ( ind > 0 );

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
        for (size_t i = 0; i < types.Size(); ++i)
        {
          uint32_t type = types[i];
          ftype::TruncValue(type, 2);

          if (find(vec.begin(), vec.end(), type) != vec.end())
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
        FillMatch(arrStreet, m_streets);
        FillMatch(arrBuilding, m_buildings);
      }

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

        for (size_t i = 0; i < types.Size(); ++i)
        {
          uint32_t type = types[i];
          ftype::TruncValue(type, 2);

          vector<uint32_t>::const_iterator j = find(m_localities.begin(), m_localities.end(), type);
          if (j != m_localities.end())
            return arrF[distance(m_localities.begin(), j)];
        }

        return 1.0;
      }
    };

    TypeChecker const & m_checker;
    bool m_doLocalities;

  protected:
    virtual double GetResultDistance(double d, feature::TypesHolder const & types) const
    {
      if (!m_doLocalities)
        return (d + GetCompareEpsilonImpl(types.GetGeoType(), 5.0 * MercatorBounds::degreeInMetres));
      else
      {
        // This routine is needed for quality of locality prediction.
        // Hamlet may be the nearest point, but it's a part of a City. So use the divide factor
        // for distance, according to feature type.
        return (d / m_checker.GetLocalityDivideFactor(types));
      }
    }
    virtual double NeedProcess(feature::TypesHolder const & types) const
    {
      if (!DoGetFeatureInfoBase::NeedProcess(types))
        return false;

      return (!m_doLocalities ||
              (types.GetGeoType() == feature::GEOM_POINT && m_checker.IsLocality(types)));
    }

  public:
    DoGetAddressInfo(m2::PointD const & pt, double eps, int scale,
                     TypeChecker const & checker)
      : DoGetFeatureInfoBase(pt, eps, scale),
        m_checker(checker), m_doLocalities(false)
    {
    }

    void PrepareForLocalities()
    {
      m_doLocalities = true;
      m_cont.clear();
    }

    void FillAddress(Framework::AddressInfo & info)
    {
      SortResults();

      for (size_t i = 0; i < m_cont.size(); ++i)
      {
        bool const isStreet = m_checker.IsStreet(m_cont[i].m_types);
        //bool const isBuilding = m_checker.IsBuilding(m_cont[i].m_types);

        if (info.m_street.empty() && isStreet)
          info.m_street = m_cont[i].m_name;

        //if (info.m_house.empty() && isBuilding)
        //  info.m_house = m_cont[i].m_house;

        if (info.m_name.empty())
        {
          if (m_cont[i].m_types.GetGeoType() != feature::GEOM_LINE)
            info.m_name = m_cont[i].m_name;
        }

        if (!(info.m_street.empty() || info.m_name.empty()))
          break;
      }
    }

    void FillLocality(Framework::AddressInfo & info, Framework const & fm)
    {
      SortResults();

      for (size_t i = 0; i < m_cont.size(); ++i)
      {
        if (!m_cont[i].m_name.empty() && fm.GetCountryName(m_cont[i].m_pt) == info.m_country)
        {
          info.m_city = m_cont[i].m_name;
          break;
        }
      }
    }
  };
}

void Framework::GetAddressInfo(m2::PointD const & pt, AddressInfo & info) const
{
  info.m_country = GetCountryName(pt);
  if (info.m_country.empty())
  {
    LOG(LINFO, ("Can't find region for point ", pt));
    return;
  }

  int const scale = scales::GetUpperScale();
  double const addressR = 200.0;
  double const localityR = 20000;

  static DoGetAddressInfo::TypeChecker checker;

  // first of all - get an address
  {
    DoGetAddressInfo getAddress(pt, addressR, scale, checker);

    m_model.ForEachFeature(
          MercatorBounds::RectByCenterXYAndSizeInMeters(pt, addressR), getAddress, scale);

    getAddress.FillAddress(info);
  }

  // now - get the locality
  {
    DoGetAddressInfo getLocality(pt, localityR, scale, checker);
    getLocality.PrepareForLocalities();

    m_model.ForEachFeature(
          MercatorBounds::RectByCenterXYAndSizeInMeters(pt, localityR), getLocality, scale);

    getLocality.FillLocality(info, *this);
  }
}
