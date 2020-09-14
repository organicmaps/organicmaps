protocol PlacePageInteractorProtocol: class {
  func updateTopBound(_ bound: CGFloat, duration: TimeInterval)
}

class PlacePageInteractor {
  weak var presenter: PlacePagePresenterProtocol?
  weak var viewController: UIViewController?
  weak var mapViewController: MapViewController?

  private var placePageData: PlacePageData

  init (viewController: UIViewController, data: PlacePageData, mapViewController: MapViewController) {
    self.placePageData = data
    self.viewController = viewController
    self.mapViewController = mapViewController
  }
}

extension PlacePageInteractor: PlacePageInteractorProtocol {
  func updateTopBound(_ bound: CGFloat, duration: TimeInterval) {
    mapViewController?.setPlacePageTopBound(bound, duration: duration)
  }
}

// MARK: - PlacePagePreviewViewControllerDelegate

extension PlacePageInteractor: PlacePagePreviewViewControllerDelegate {
  func previewDidPressRemoveAds() {
    MWMPlacePageManagerHelper.showRemoveAds()
  }

  func previewDidPressAddReview() {
    MWMPlacePageManagerHelper.showUGCAddReview(placePageData, rating: .none, from: .placePagePreview)
  }

  func previewDidPressSimilarHotels() {
    MWMPlacePageManagerHelper.searchSimilar(placePageData)
  }
}

// MARK: - PlacePageInfoViewControllerDelegate

extension PlacePageInteractor: PlacePageInfoViewControllerDelegate {
  func didPressCall() {
    MWMPlacePageManagerHelper.call(placePageData)
  }

  func didPressWebsite() {
    MWMPlacePageManagerHelper.openWebsite(placePageData)
  }

  func didPressEmail() {

  }

  func didPressLocalAd() {
    MWMPlacePageManagerHelper.openLocalAdsURL(placePageData)
  }
}

// MARK: - WikiDescriptionViewControllerDelegate

extension PlacePageInteractor: WikiDescriptionViewControllerDelegate {
  func didPressMore() {
    MWMPlacePageManagerHelper.showPlaceDescription(placePageData.wikiDescriptionHtml)
  }
}

// MARK: - TaxiViewControllerDelegate

extension PlacePageInteractor: TaxiViewControllerDelegate {
  func didPressOrder() {
    MWMPlacePageManagerHelper.orderTaxi(placePageData)
  }
}

// MARK: - AddReviewViewControllerDelegate

extension PlacePageInteractor: AddReviewViewControllerDelegate {
  func didRate(_ rating: UgcSummaryRatingType) {
    MWMPlacePageManagerHelper.showUGCAddReview(placePageData, rating: rating, from: .placePage)
  }
}

// MARK: - PlacePageReviewsViewControllerDelegate

extension PlacePageInteractor: PlacePageReviewsViewControllerDelegate {
  func didPressMoreReviews() {
    let moreReviews = viewController!.storyboard!.instantiateViewController(ofType: MoreReviewsViewController.self)
    moreReviews.ugcData = placePageData.ugcData
    moreReviews.title = placePageData.previewData.title
    MapViewController.shared()?.navigationController?.pushViewController(moreReviews, animated: true)
  }
}

// MARK: - PlacePageButtonsViewControllerDelegate

extension PlacePageInteractor: PlacePageButtonsViewControllerDelegate {
  func didPressHotels() {
    MWMPlacePageManagerHelper.openDescriptionUrl(placePageData)
  }

  func didPressAddPlace() {
    MWMPlacePageManagerHelper.addPlace(placePageData.locationCoordinate)
  }

  func didPressEditPlace() {
    MWMPlacePageManagerHelper.editPlace()
  }

  func didPressAddBusiness() {
    MWMPlacePageManagerHelper.addBusiness()
  }
}

// MARK: - HotelPhotosViewControllerDelegate

extension PlacePageInteractor: HotelPhotosViewControllerDelegate {
  func didSelectItemAt(_ hotelPhotosViewController: HotelPhotosViewController, index: Int, lastItemIndex: Int) {
    guard let photos = placePageData.hotelBooking?.photos else { return }
    if index == lastItemIndex {
      let galleryController = GalleryViewController.instance(photos: photos)
      galleryController.title = placePageData.previewData.title
      MapViewController.shared()?.navigationController?.pushViewController(galleryController, animated: true)
    } else {
      let currentPhoto = photos[index]
      let view = hotelPhotosViewController.viewForPhoto(currentPhoto)
      let photoVC = PhotosViewController(photos: photos, initialPhoto: currentPhoto, referenceView: view)

      photoVC.referenceViewForPhotoWhenDismissingHandler = {
        hotelPhotosViewController.viewForPhoto($0)
      }
      viewController?.present(photoVC, animated: true)
    }
  }
}

// MARK: - HotelDescriptionViewControllerDelegate

extension PlacePageInteractor: HotelDescriptionViewControllerDelegate {
  func hotelDescriptionDidPressMore() {
    MWMPlacePageManagerHelper.openMoreUrl(placePageData)
  }
}

// MARK: - HotelFacilitiesViewControllerDelegate

extension PlacePageInteractor: HotelFacilitiesViewControllerDelegate {
  func facilitiesDidPressMore() {
    MWMPlacePageManagerHelper.showAllFacilities(placePageData)
  }
}

// MARK: - HotelReviewsViewControllerDelegate

extension PlacePageInteractor: HotelReviewsViewControllerDelegate {
  func hotelReviewsDidPressMore() {
    MWMPlacePageManagerHelper.openReviewUrl(placePageData)
  }
}

// MARK: - CatalogSingleItemViewControllerDelegate

extension PlacePageInteractor: CatalogSingleItemViewControllerDelegate {
  func catalogPromoItemDidPressView() {
    MWMPlacePageManagerHelper.openCatalogSingleItem(placePageData, at: 0)
  }

  func catalogPromoItemDidPressMore() {
    MWMPlacePageManagerHelper.openCatalogSingleItem(placePageData, at: 0)
  }
}

// MARK: - CatalogGalleryViewControllerDelegate

extension PlacePageInteractor: CatalogGalleryViewControllerDelegate {
  func promoGalleryDidPressMore() {
    MWMPlacePageManagerHelper.openCatalogMoreItems(placePageData)
  }

  func promoGalleryDidSelectItemAtIndex(_ index: Int) {
    MWMPlacePageManagerHelper.openCatalogSingleItem(placePageData, at: index)
  }
}

// MARK: - PlacePageBookmarkViewControllerDelegate

extension PlacePageInteractor: PlacePageBookmarkViewControllerDelegate {
  func bookmarkDidPressEdit() {
    MWMPlacePageManagerHelper.editBookmark(placePageData)
  }
}

// MARK: - ActionBarViewControllerDelegate

extension PlacePageInteractor: ActionBarViewControllerDelegate {
  func actionBar(_ actionBar: ActionBarViewController, didPressButton type: ActionBarButtonType) {
    switch type {
    case .booking:
      MWMPlacePageManagerHelper.book(placePageData)
    case .bookingSearch:
      MWMPlacePageManagerHelper.searchBookingHotels(placePageData)
    case .bookmark:
      if placePageData.bookmarkData != nil {
        MWMPlacePageManagerHelper.removeBookmark(placePageData)
      } else {
        MWMPlacePageManagerHelper.addBookmark(placePageData)
      }
    case .call:
      MWMPlacePageManagerHelper.call(placePageData)
    case .download:
      guard let mapNodeAttributes = placePageData.mapNodeAttributes else {
        fatalError("Download button can't be displayed if mapNodeAttributes is empty")
      }
      switch mapNodeAttributes.nodeStatus {
      case .downloading, .inQueue, .applying:
        Storage.shared().cancelDownloadNode(mapNodeAttributes.countryId)
      case .notDownloaded, .partly, .error:
        Storage.shared().downloadNode(mapNodeAttributes.countryId)
      case .undefined, .onDiskOutOfDate, .onDisk:
        fatalError("Download button shouldn't be displayed when node is in these states")
      @unknown default:
        fatalError()
      }
    case .opentable:
      fatalError("Opentable is not supported and will be deleted")
    case .partner:
      MWMPlacePageManagerHelper.openPartner(placePageData)
    case .routeAddStop:
      MWMPlacePageManagerHelper.routeAddStop(placePageData)
    case .routeFrom:
      MWMPlacePageManagerHelper.route(from: placePageData)
    case .routeRemoveStop:
      MWMPlacePageManagerHelper.routeRemoveStop(placePageData)
    case .routeTo:
      MWMPlacePageManagerHelper.route(to: placePageData)
    case .share:
      let shareVC = ActivityViewController.share(forPlacePage: placePageData)
      shareVC!.present(inParentViewController: MapViewController.shared(), anchorView: actionBar.popoverSourceView)
      Statistics.logEvent(kStatEventName(kStatPlacePage, kStatShare))
    case .avoidToll:
      MWMPlacePageManagerHelper.avoidToll()
    case .avoidDirty:
      MWMPlacePageManagerHelper.avoidDirty()
    case .avoidFerry:
      MWMPlacePageManagerHelper.avoidFerry()
    case .more:
      fatalError("More button should've been handled in ActionBarViewContoller")
    @unknown default:
      fatalError()
    }
  }
}

// MARK: - ElevationProfileViewControllerDelegate

extension PlacePageInteractor: ElevationProfileViewControllerDelegate {
  func openDifficultyPopup() {
    MWMPlacePageManagerHelper.openElevationDifficultPopup(placePageData)
  }

  func updateMapPoint(_ distance: Double) {
    BookmarksManager.shared().setElevationActivePoint(distance, trackId: placePageData.elevationProfileData!.trackId)
  }
}

// MARK: - PlacePageHeaderViewController

extension PlacePageInteractor: PlacePageHeaderViewControllerDelegate {
  func previewDidPressClose() {
    presenter?.closeAnimated()
  }

  func previewDidPressExpand() {
    presenter?.showNextStop()
  }
}
