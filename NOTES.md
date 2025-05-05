# Loading earth chunks:
- load at same time when calling `m_work.LoadMapsAsync(std::move(afterMapsLoaded));` in Framework initialization?

# Fetching features:

```cpp
void Framework::VisualizeRoadsInRect(m2::RectD const & rect)
{
  m_featuresFetcher.ForEachFeature(rect, [this, &rect](FeatureType & ft)
  {
    if (routing::IsRoad(feature::TypesHolder(ft)))
      VisualizeFeatureInRect(rect, ft, m_drapeApi);
  }, scales::GetUpperScale());
}
```

```cpp
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
```
