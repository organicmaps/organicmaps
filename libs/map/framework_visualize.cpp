#include "framework.hpp"

#include "routing/city_roads.hpp"
#include "routing/cross_mwm_index_graph.hpp"

namespace
{
dp::Color const colorList[] = {{255, 0, 0, 255},   {0, 255, 0, 255},   {0, 0, 255, 255},   {255, 255, 0, 255},
                               {0, 255, 255, 255}, {255, 0, 255, 255}, {100, 0, 0, 255},   {0, 100, 0, 255},
                               {0, 0, 100, 255},   {100, 100, 0, 255}, {0, 100, 100, 255}, {100, 0, 100, 255}};

dp::Color const cityBoundaryBBColor{255, 0, 0, 255};
dp::Color const cityBoundaryCBColor{0, 255, 0, 255};
dp::Color const cityBoundaryDBColor{0, 0, 255, 255};

template <class Box>
void DrawLine(Box const & box, dp::Color const & color, df::DrapeApi & drapeApi, std::string const & id)
{
  auto points = box.Points();
  CHECK(!points.empty(), ());
  points.push_back(points.front());

  points.erase(unique(points.begin(), points.end(),
                      [](m2::PointD const & p1, m2::PointD const & p2)
  {
    m2::PointD const delta = p2 - p1;
    return delta.IsAlmostZero();
  }),
               points.end());

  if (points.size() <= 1)
    return;

  drapeApi.AddLine(id, df::DrapeApiLineData(points, color).Width(3.0f).ShowPoints(true).ShowId());
}

void VisualizeFeatureInRect(m2::RectD const & rect, FeatureType & ft, df::DrapeApi & drapeApi)
{
  bool allPointsOutside = true;
  std::vector<m2::PointD> points;
  ft.ForEachPoint([&](m2::PointD const & pt)
  {
    if (rect.IsPointInside(pt))
      allPointsOutside = false;
    points.push_back(pt);
  }, scales::GetUpperScale());

  if (!allPointsOutside)
  {
    static uint64_t counter = 0;
    auto const color = colorList[counter++ % std::size(colorList)];

    // Note. The first param at DrapeApi::AddLine() should be unique, so pass unique ft.GetID().
    // Other way last added line replaces the previous added line with the same name.
    drapeApi.AddLine(DebugPrint(ft.GetID()), df::DrapeApiLineData(points, color).Width(3.0f).ShowPoints(true).ShowId());
  }
}
}  // namespace

void Framework::VisualizeRoadsInRect(m2::RectD const & rect)
{
  m_featuresFetcher.ForEachFeature(rect, [this, &rect](FeatureType & ft)
  {
    if (routing::IsRoad(feature::TypesHolder(ft)))
      VisualizeFeatureInRect(rect, ft, m_drapeApi);
  }, scales::GetUpperScale());
}

void Framework::VisualizeCityBoundariesInRect(m2::RectD const & rect)
{
  DataSource const & dataSource = GetDataSource();
  search::CitiesBoundariesTable table(dataSource);
  table.Load();

  std::vector<uint32_t> featureIds;
  GetCityBoundariesInRectForTesting(table, rect, featureIds);

  FeaturesLoaderGuard loader(dataSource, dataSource.GetMwmIdByCountryFile(platform::CountryFile("World")));
  for (auto const fid : featureIds)
  {
    search::CitiesBoundariesTable::Boundaries boundaries;
    if (!table.Get(fid, boundaries))
      continue;

    std::string id = "fid:" + strings::to_string(fid);
    auto ft = loader.GetFeatureByIndex(fid);
    if (ft)
    {
      auto name = ft->GetName(StringUtf8Multilang::kEnglishCode);
      if (name.empty())
        name = ft->GetName(StringUtf8Multilang::kDefaultCode);
      id.append(", name:").append(name);
    }

    boundaries.ForEachBoundary([&id, this](indexer::CityBoundary const & cityBoundary, size_t i)
    {
      std::string idWithIndex = id;
      if (i > 0)
        idWithIndex = id + ", i:" + strings::to_string(i);

      DrawLine(cityBoundary.m_bbox, cityBoundaryBBColor, m_drapeApi, idWithIndex + ", bb");
      DrawLine(cityBoundary.m_cbox, cityBoundaryCBColor, m_drapeApi, idWithIndex + ", cb");
      DrawLine(cityBoundary.m_dbox, cityBoundaryDBColor, m_drapeApi, idWithIndex + ", db");
    });
  }
}

void Framework::VisualizeCityRoadsInRect(m2::RectD const & rect)
{
  std::map<MwmSet::MwmId, std::unique_ptr<routing::CityRoads>> cityRoads;
  GetDataSource().ForEachInRect([&](FeatureType & ft)
  {
    if (ft.GetGeomType() != feature::GeomType::Line)
      return;

    auto const & mwmId = ft.GetID().m_mwmId;
    auto const it = cityRoads.find(mwmId);
    if (it == cityRoads.cend())
    {
      MwmSet::MwmHandle handle = m_featuresFetcher.GetDataSource().GetMwmHandleById(mwmId);
      CHECK(handle.IsAlive(), ());

      cityRoads[mwmId] = routing::LoadCityRoads(handle);
    }

    if (cityRoads[mwmId]->IsCityRoad(ft.GetID().m_index))
      VisualizeFeatureInRect(rect, ft, m_drapeApi);
  }, rect, scales::GetUpperScale());
}

void Framework::VisualizeCrossMwmTransitionsInRect(m2::RectD const & rect)
{
  using CrossMwmID = base::GeoObjectId;
  using ConnectorT = routing::CrossMwmConnector<CrossMwmID>;
  std::map<MwmSet::MwmId, ConnectorT> connectors;
  std::map<MwmSet::MwmId, dp::Color> colors;

  GetDataSource().ForEachInRect([&](FeatureType & ft)
  {
    if (ft.GetGeomType() != feature::GeomType::Line)
      return;

    auto const & mwmId = ft.GetID().m_mwmId;
    auto res = connectors.try_emplace(mwmId, ConnectorT());
    ConnectorT & connector = res.first->second;
    if (res.second)
    {
      MwmSet::MwmHandle handle = m_featuresFetcher.GetDataSource().GetMwmHandleById(mwmId);
      CHECK(handle.IsAlive(), ());

      auto reader = routing::connector::GetReader<CrossMwmID>(handle.GetValue()->m_cont);
      routing::CrossMwmConnectorBuilder<CrossMwmID> builder(connector);
      builder.DeserializeTransitions(routing::VehicleType::Car, reader);

      static uint32_t counter = 0;
      colors.emplace(mwmId, colorList[counter++ % std::size(colorList)]);
    }

    std::vector<uint32_t> transitSegments;
    connector.ForEachTransitSegmentId(ft.GetID().m_index, [&transitSegments](uint32_t seg)
    {
      transitSegments.push_back(seg);
      return false;
    });

    if (!transitSegments.empty())
    {
      auto const color = colors.find(mwmId)->second;

      int segIdx = -1;
      m2::PointD prevPt;
      ft.ForEachPoint([&](m2::PointD const & pt)
      {
        if (base::IsExist(transitSegments, segIdx))
        {
          GetDrapeApi().AddLine(DebugPrint(ft.GetID()) + ", " + std::to_string(segIdx),
                                df::DrapeApiLineData({prevPt, pt}, color).Width(10.0f));
        }

        prevPt = pt;
        ++segIdx;
      }, scales::GetUpperScale());
    }
  }, rect, scales::GetUpperScale());
}
