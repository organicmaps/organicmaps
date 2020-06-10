import Foundation

protocol IGuidesGalleryPresenter {
  func viewDidLoad()
  func selectItemAtIndex(_ index: Int)
  func scrollToItemAtIndex(_ index: Int)
  func toggleVisibilityAtIndex(_ index: Int)
}

final class GuidesGalleryPresenter {
  private unowned let view: IGuidesGalleryView
  private let router: IGuidesGalleryRouter
  private let interactor: IGuidesGalleryInteractor
  private var galleryItems: [GuidesGalleryItem] = []
  private let formatter = ChartFormatter(imperial: Settings.measurementUnits() == .imperial)

  init(view: IGuidesGalleryView, router: IGuidesGalleryRouter, interactor: IGuidesGalleryInteractor) {
    self.view = view
    self.router = router
    self.interactor = interactor
  }

  deinit {
    interactor.resetGalleryChangedCallback()
  }

  private func makeViewModel(_ item: GuidesGalleryItem) -> IGuidesGalleryItemViewModel {
    switch item {
    case let cityItem as CityGalleryItem:
      return makeCityItemViewModel(cityItem)
    case let outdoorItem as OutdoorGalleryItem:
      return makeOutdoorItemViewModel(outdoorItem)
    default:
      fatalError("Unexpected item type \(item)")
    }
  }

  private func makeCityItemViewModel(_ item: CityGalleryItem) -> IGuidesGalleryCityItemViewModel {
    var model = GalleryCityItemViewModel(item)
    if model.downloaded {
      model.visible = interactor.isGalleryItemVisible(item)
    }
    return model
  }

  private func makeOutdoorItemViewModel(_ item: OutdoorGalleryItem) -> IGuidesGalleryOutdoorItemViewModel {
    var model = GalleryOutdoorItemViewModel(item, formatter: formatter)
    if model.downloaded {
      model.visible = interactor.isGalleryItemVisible(item)
    }
    return model
  }

  private func reloadGallery() {
    galleryItems = interactor.galleryItems()
    view.setGalleryItems(galleryItems.map({ makeViewModel($0) }))
  }

  private func setActiveItem() {
    let activeGuideId = self.interactor.activeItemId()
    guard let activeIndex = galleryItems.firstIndex(where: {
      $0.guideId == activeGuideId
    }) else { return }
    view.setActiveItem(activeIndex, animated: false)
    logShowItemAtIndex(activeIndex)
  }

  private func logShowItemAtIndex(_ index: Int) {
    let item = galleryItems[index]
    guard item.downloaded else { return }
    Statistics.logEvent(kStatPlacepageSponsoredUserItemShown,
                        withParameters: [kStatProvider : kStatMapsmeGuides,
                                         kStatPlacement : kStatMap,
                                         kStatState : kStatOnline,
                                         kStatItem : index,
                                         kStatId : item.guideId],
                        with: .realtime)
  }
}

extension GuidesGalleryPresenter: IGuidesGalleryPresenter {
  func viewDidLoad() {
    reloadGallery()
    setActiveItem()

    interactor.setGalleryChangedCallback { [weak self] (reloadGallery) in
      if reloadGallery {
        self?.reloadGallery()
      }
      self?.setActiveItem()
    }


    Statistics.logEvent(kStatPlacepageSponsoredShow, withParameters: [kStatProvider : kStatMapsmeGuides,
                                                                      kStatPlacement : kStatMap,
                                                                      kStatState : kStatOnline,
                                                                      kStatCount : galleryItems.count])
  }

  func selectItemAtIndex(_ index: Int) {
    let galleryItem = galleryItems[index]
    guard let url = URL(string: galleryItem.url) else { return }
    router.openCatalogUrl(url)
    Statistics.logEvent(kStatPlacepageSponsoredItemSelected, withParameters: [kStatProvider : kStatMapsmeGuides,
                                                                              kStatPlacement : kStatMap,
                                                                              kStatItem : index,
                                                                              kStatDestination : kStatCatalogue])
  }

  func scrollToItemAtIndex(_ index: Int) {
    let galleryItem = galleryItems[index]
    interactor.setActiveItem(galleryItem)
    logShowItemAtIndex(index)
  }

  func toggleVisibilityAtIndex(_ index: Int) {
    let galleryItem = galleryItems[index]
    interactor.toggleItemVisibility(galleryItem)
    let model = makeViewModel(galleryItem)
    view.updateItem(model, at: index)
  }
}

fileprivate struct GalleryCityItemViewModel: IGuidesGalleryCityItemViewModel {
  var title: String
  var subtitle: String
  var imageUrl: URL?
  var downloaded: Bool
  var visible: Bool?
  var info: String

  init(_ item: CityGalleryItem) {
    title = item.title
    subtitle = item.hasTrack ? L("routes_card_routes_tag") : L("routes_card_set_tag")
    imageUrl = URL(string: item.imageUrl)
    downloaded = item.downloaded
    var infoString = String(coreFormat: L("routes_card_number_of_points"), arguments: [item.bookmarksCount])
    if item.hasTrack {
      infoString.append(" \(L("routes_card_plus_track"))")
    }
    info = infoString
  }
}

fileprivate struct GalleryOutdoorItemViewModel: IGuidesGalleryOutdoorItemViewModel {
  var title: String
  var subtitle: String
  var imageUrl: URL?
  var downloaded: Bool
  var visible: Bool?
  var distance: String
  var duration: String?
  var ascent: String

  init(_ item: OutdoorGalleryItem, formatter: ChartFormatter) {
    title = item.title
    subtitle = item.tag
    imageUrl = URL(string: item.imageUrl)
    downloaded = item.downloaded
    duration = item.duration > 0 ? formatter.timeString(from: Double(item.duration)) : nil
    distance = formatter.distanceString(from: item.distance)
    ascent = formatter.altitudeString(from: Double(item.ascent))
  }
}
