import Foundation

protocol IGuidesGalleryRouter {
  func openCatalogUrl(_ url: URL)
}

final class GuidesGalleryRouter {
  private let mapViewController: MapViewController

  init(_ mapViewController: MapViewController) {
    self.mapViewController = mapViewController
  }
}

extension GuidesGalleryRouter: IGuidesGalleryRouter {
  func openCatalogUrl(_ url: URL) {
    mapViewController.openCatalogAbsoluteUrl(url, animated: true, utm: .guidesOnMapGallery)
  }
}
