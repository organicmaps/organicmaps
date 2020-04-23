import Foundation

protocol IGuidesGalleryPresenter {
  func viewDidLoad()
  func selectItemAtIndex(_ index: Int)
}

final class GuidesGalleryPresenter {
  private weak var view: IGuidesGalleryView?
  private var guidesGallery: GuidesGalleryData
  private let formatter = ChartFormatter(imperial: Settings.measurementUnits() == .imperial)

  init(view: IGuidesGalleryView, guidesGallery: GuidesGalleryData) {
    self.view = view
    self.guidesGallery = guidesGallery
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
    GalleryCityItemViewModel(item)
  }

  private func makeOutdoorItemViewModel(_ item: OutdoorGalleryItem) -> IGuidesGalleryOutdoorItemViewModel {
    GalleryOutdoorItemViewModel(item, formatter: formatter)
  }
}

extension GuidesGalleryPresenter: IGuidesGalleryPresenter {
  func viewDidLoad() {
    view?.setGalleryItems(guidesGallery.galleryItems.map({ makeViewModel($0) }))
  }

  func selectItemAtIndex(_ index: Int) {

  }
}

fileprivate struct GalleryCityItemViewModel: IGuidesGalleryCityItemViewModel {
  var title: String
  var subtitle: String
  var imageUrl: URL?
  var downloaded: Bool
  var info: String

  init(_ item: CityGalleryItem) {
    title = item.title
    subtitle = item.hasTrack ? L("routes_card_routes_tag") : L("routes_card_set_tag")
    imageUrl = URL(string: item.imageUrl)
    downloaded = item.downloaded
    var infoString = String(coreFormat: "routes_card_number_of_points", arguments: [item.bookmarksCount])
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
