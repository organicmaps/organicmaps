#include "address_enricher.hpp"

#include "generator/feature_maker_base.hpp"

#include "search/house_numbers_matcher.hpp"

#include "indexer/ftypes_matcher.hpp"
#include "indexer/search_string_utils.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/mercator.hpp"
#include "geometry/parametrized_segment.hpp"

#include "coding/point_coding.hpp"

namespace generator
{
using feature::InterpolType;

std::pair<uint64_t, uint64_t> AddressEnricher::RawEntryBase::GetHNRange() const
{
  using namespace search::house_numbers;

  std::vector<TokensT> from, to;
  ParseHouseNumber(strings::MakeUniString(m_from), from);
  ParseHouseNumber(strings::MakeUniString(m_to), to);

  if (from.size() != 1 || to.size() != 1)
    return kInvalidRange;

  ASSERT(!from[0].empty() && !to[0].empty(), ());
  auto const & f = from[0][0];
  auto const & t = to[0][0];
  if (f.m_type != Token::TYPE_NUMBER || t.m_type != Token::TYPE_NUMBER)
    return kInvalidRange;

  return {ToUInt(f.m_value), ToUInt(t.m_value)};
}

std::string DebugPrint(AddressEnricher::Stats const & s)
{
  return "{ m_noStreet = " + std::to_string(s.m_noStreet) + "; m_existInterpol = " + std::to_string(s.m_existInterpol) +
         "; m_existSingle = " + std::to_string(s.m_existSingle) +
         "; m_enoughAddrs = " + std::to_string(s.m_enoughAddrs) +
         "; m_addedSingle = " + std::to_string(s.m_addedSingle) +
         "; m_addedBegEnd = " + std::to_string(s.m_addedBegEnd) +
         "; m_addedInterpol = " + std::to_string(s.m_addedInterpol) + " }";
}

AddressEnricher::AddressEnricher()
{
  auto const & cl = classif();
  m_addrType = cl.GetTypeByPath({"building", "address"});

  m_interpolType[InterpolType::Odd] = cl.GetTypeByPath({"addr:interpolation", "odd"});
  m_interpolType[InterpolType::Even] = cl.GetTypeByPath({"addr:interpolation", "even"});
  m_interpolType[InterpolType::Any] = cl.GetTypeByPath({"addr:interpolation"});
}

void AddressEnricher::AddSrc(feature::FeatureBuilder && fb)
{
  auto const osmID = fb.GetMostGenericOsmId();
  auto const & params = fb.GetParams();
  auto const & types = fb.GetTypes();

  bool const hasStreet = !params.GetStreet().empty();

  if (hasStreet && !params.house.IsEmpty())
  {
    if (fb.IsLine())
    {
      LOG(LERROR, ("Linear address FB:", osmID));
      return;
    }
    else
      TransformToPoint(fb);
  }
  else if (ftypes::IsStreetOrSquareChecker::Instance()(types))
  {
    // Skip node "squares" for matching, as far as we make addr:interpolation FBs.
    // https://www.openstreetmap.org/node/7315145526
    if (fb.IsPoint())
    {
      LOG(LWARNING, ("Point street FB:", osmID));
      return;
    }

    std::string_view name;
    if (params.name.GetString(StringUtf8Multilang::kDefaultCode, name))
      CHECK(!name.empty(), (osmID));
    else if (params.ref.empty())
      return;
  }
  else if (hasStreet && ftypes::IsAddressInterpolChecker::Instance()(types))
  {
    CHECK(!params.ref.empty(), (osmID));
  }
  else
    return;

  m_srcTree.Add(std::move(fb));
}

void AddressEnricher::ProcessRawEntries(std::string const & path, TFBCollectFn const & fn)
{
  FileReader reader(path);
  ReaderSource src(reader);
  while (src.Size())
  {
    Entry e;
    e.Load(src);

    std::vector<int64_t> iPoints;
    rw::Read(src, iPoints);

    e.m_points.reserve(iPoints.size());
    for (auto const i : iPoints)
      e.m_points.push_back(Int64ToPointObsolete(i, kPointCoordBits));

    auto const res = Match(e);
    if (!res.street)
    {
      ++m_stats.m_noStreet;
      LOG(LWARNING, ("No street found:", e.m_street, mercator::ToLatLon(e.m_points.front())));
      continue;
    }
    if (res.interpol)
    {
      ++m_stats.m_existInterpol;
      continue;
    }

    auto const addNode = [&](m2::PointD const & p, std::string hn)
    {
      feature::FeatureBuilder fb;
      fb.SetCenter(p);
      fb.SetType(m_addrType);

      auto & params = fb.GetParams();
      params.SetStreet(e.m_street);
      params.SetPostcode(e.m_postcode);

      CHECK(!hn.empty(), ());
      params.SetHouseNumberAndHouseName(std::move(hn), {});
      params.SetGeomTypePointEx();

      fn(std::move(fb));
    };

    if (e.m_points.size() == 1)
    {
      if (!res.from && !res.to && res.addrsInside == 0)
      {
        ++m_stats.m_addedSingle;
        addNode(e.m_points.front(), e.m_from + " - " + e.m_to);
      }
      else
        ++m_stats.m_existSingle;
    }
    else
    {
      if (res.addrsInside > 0)
      {
        double const distM = mercator::DistanceOnEarth(e.m_points.front(), e.m_points.back());
        if (distM / kDistanceThresholdM <= res.addrsInside)
        {
          ++m_stats.m_enoughAddrs;
          continue;
        }
      }

      if (!res.from)
      {
        addNode(e.m_points.front(), e.m_from);
        ++m_stats.m_addedBegEnd;
      }
      if (!res.to)
      {
        addNode(e.m_points.back(), e.m_to);
        ++m_stats.m_addedBegEnd;
      }

      // Add interpolation FB.
      feature::FeatureBuilder fb;
      fb.AssignPoints(std::move(e.m_points));
      fb.SetLinear();
      CHECK(e.m_interpol != InterpolType::None, ());
      fb.SetType(m_interpolType[e.m_interpol]);

      // Same as in AddressesHolder::Update.
      auto & params = fb.GetParams();
      params.SetStreet(e.m_street);
      params.SetPostcode(e.m_postcode);
      params.ref = e.m_from + ":" + e.m_to;

      fn(std::move(fb));
      ++m_stats.m_addedInterpol;
    }
  }
}

AddressEnricher::FoundT AddressEnricher::Match(Entry & e) const
{
  // Calc query rect with the threshold.
  m2::RectD rect;
  for (auto const & p : e.m_points)
    rect.Add(p);
  m2::RectD const inflation = mercator::RectByCenterXYAndSizeInMeters(rect.Center(), kDistanceThresholdM);
  rect.Inflate(inflation.SizeX(), inflation.SizeY());

  // Tiger usually has short (St) names, while OSM has full (Street) names.
  auto const streetKey = search::GetNormalizedStreetName(e.m_street);

  // Get HN range for the entry.
  auto const range = e.GetHNRange();
  CHECK(range != Entry::kInvalidRange, ());
  CHECK(range.first <= range.second, (range));

  // Prepare distance calculator for the entry.
  std::vector<m2::ParametrizedSegment<m2::PointD>> eSegs;
  eSegs.reserve(e.m_points.size() - 1);
  for (size_t i = 1; i < e.m_points.size(); ++i)
    eSegs.emplace_back(e.m_points[i - 1], e.m_points[i]);

  /// @todo Check nodes distance for now. Should make more honest algo.
  auto const isClose = [&e, &eSegs](feature::FeatureBuilder const & fb)
  {
    bool res = false;

    fb.ForEachPoint([&](m2::PointD const & p)
    {
      if (res)
        return;

      auto const ll = mercator::ToLatLon(p);
      auto const check = [&ll](m2::PointD const & p)
      { return ms::DistanceOnEarth(ll, mercator::ToLatLon(p)) < kDistanceThresholdM; };

      if (!eSegs.empty())
      {
        for (auto const & seg : eSegs)
          if (check(seg.ClosestPointTo(p)))
          {
            res = true;
            return;
          }
      }
      else
        res = check(e.m_points.front());
    });

    return res;
  };

  FoundT res;

  m_srcTree.ForEachInRect(rect, [&](feature::FeatureBuilder const & fb)
  {
    auto const osmID = fb.GetMostGenericOsmId();
    auto const & params = fb.GetParams();
    auto const & types = fb.GetTypes();
    bool const isStreet = ftypes::IsStreetOrSquareChecker::Instance()(types);

    // First of all - compare street's name.
    strings::UniString fbStreetKey = search::GetNormalizedStreetName(params.GetStreet());
    std::string streetName;  // original street's name, valid if isStreet == true
    if (isStreet)
    {
      // Fancy object, highway=pedestrian with addr: https://www.openstreetmap.org/way/415336229
      if (!fbStreetKey.empty())
        LOG(LWARNING, ("Street with address:", osmID));

      // Take 'ref' if 'name' is empty, like here: https://www.openstreetmap.org/way/902910704
      std::string_view name;
      if (params.name.GetString(StringUtf8Multilang::kDefaultCode, name))
        streetName = name;
      else if (!params.ref.empty())
        streetName = params.ref;

      if (!streetName.empty())
        fbStreetKey = search::GetNormalizedStreetName(streetName);
    }
    if (streetKey != fbStreetKey)
      return;

    if (!params.house.IsEmpty())
    {
      if (!isClose(fb))
        return;

      using namespace search::house_numbers;
      std::vector<TokensT> tokens;
      ParseHouseNumber(strings::MakeUniString(params.house.Get()), tokens);

      // Iterate via all possible int values (eg: 100-200;202;204).
      for (auto const & token : tokens)
        for (auto const & t : token)
        {
          if (t.m_type == Token::TYPE_NUMBER)
          {
            uint64_t const num = ToUInt(t.m_value);
            if (range.first <= num && num <= range.second)
            {
              ++res.addrsInside;
              if (range.first == num)
                res.from = true;
              if (range.second == num)
                res.to = true;
            }
          }
        }
    }
    else if (!res.street && isStreet)
    {
      /// @todo Select the closest street if many options?

      // We can't call isClose here, should make "vice-versa" check.
      auto const & geom = fb.GetOuterGeometry();
      for (size_t i = 1; i < geom.size(); ++i)
      {
        m2::ParametrizedSegment<m2::PointD> seg(geom[i - 1], geom[i]);
        /// @todo Calculate e.m_points LL once?
        for (auto const & p : e.m_points)
          if (mercator::DistanceOnEarth(p, seg.ClosestPointTo(p)) < kDistanceThresholdM)
          {
            res.street = true;
            // Assign street's name from OSM for further search index builder matching
            // (it doesn't work like GetNormalizedStreetName, but like GetStreetNameAsKey).
            e.m_street = std::move(streetName);
            return;
          }
      }
    }
    else if (!res.interpol)
    {
      auto const interpol = ftypes::IsAddressInterpolChecker::Instance().GetInterpolType(types);
      if (interpol != InterpolType::None && isClose(fb))
      {
        if (interpol != e.m_interpol && interpol != InterpolType::Any && e.m_interpol != InterpolType::Any)
          return;

        // Compare ranges.
        auto const & hnRange = params.ref;
        size_t const i = hnRange.find(':');
        CHECK(i != std::string::npos, (hnRange));
        uint64_t left, right;
        CHECK(strings::to_uint(hnRange.substr(0, i), left) && strings::to_uint(hnRange.substr(i + 1), right),
              (hnRange));

        res.interpol = !(left >= range.second || right <= range.first);
      }
    }
  });

  return res;
}

}  // namespace generator
