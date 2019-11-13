class DeepLinkSearchStrategy: IDeepLinkHandlerStrategy{
  var deeplinkURL: DeepLinkURL
  private var data: DeepLinkSearchData

  init(url: DeepLinkURL, data: DeepLinkSearchData) {
    self.deeplinkURL = url
    self.data = data
  }

  func execute() {
    let kSearchInViewportZoom: Int32 = 16;

    // Set viewport only when cll parameter was provided in url.
    if (data.centerLat != 0.0 && data.centerLon != 0.0) {
      MapViewController.setViewport(data.centerLat, lon: data.centerLon, zoomLevel: kSearchInViewportZoom)

      // We need to update viewport for search api manually because of drape engine
      // will not notify subscribers when search view is shown.
      if (!data.isSearchOnMap) {
        data.onViewportChanged(kSearchInViewportZoom)
      }
    }

    if (data.isSearchOnMap) {
      MWMMapViewControlsManager.manager()?.searchText(onMap: data.query, forInputLocale: data.locale)
    } else {
      MWMMapViewControlsManager.manager()?.searchText(data.query, forInputLocale: data.locale)
    }
    sendStatisticsOnSuccess(type: kStatSearch)
  }
}
