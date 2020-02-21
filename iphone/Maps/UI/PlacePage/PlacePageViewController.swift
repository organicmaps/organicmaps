final class PlacePageScrollView: UIScrollView {
  override func point(inside point: CGPoint, with event: UIEvent?) -> Bool {
    return point.y > 0
  }
}

final class TouchTransparentView: UIView {
  var targetView: UIView?
  override func point(inside point: CGPoint, with event: UIEvent?) -> Bool {
    guard let targetView = targetView else {
      return super.point(inside: point, with: event)
    }
    let targetPoint = convert(point, to: targetView)
    return targetView.point(inside: targetPoint, with: event)
  }
}

enum PlacePageState {
  case closed(CGFloat)
  case preview(CGFloat)
  case previewPlus(CGFloat)
  case expanded(CGFloat)

  var offset: CGFloat {
    switch self {
    case .closed(let value):
      return value
    case .preview(let value):
      return value
    case .previewPlus(let value):
      return value
    case .expanded(let value):
      return value
    }
  }
}

@objc final class PlacePageViewController: UIViewController {
  @IBOutlet var scrollView: UIScrollView!
  @IBOutlet var stackView: UIStackView!
  @IBOutlet var actionBarContainerView: UIView!
  
  @objc var placePageData: PlacePageData!

  var beginDragging = false
  var scrollSteps: [PlacePageState] = []
  var rootViewController: MapViewController {
    MapViewController.shared()
  }

  // MARK: - UI Components

  lazy var previewViewController: PlacePagePreviewViewController = {
    let vc = storyboard!.instantiateViewController(ofType: PlacePagePreviewViewController.self)
    vc.placePagePreviewData = placePageData.previewData
    vc.delegate = self
    return vc
  } ()

  lazy var catalogSingleItemViewController: CatalogSingleItemViewController = {
    let vc = storyboard!.instantiateViewController(ofType: CatalogSingleItemViewController.self)
    vc.view.isHidden = true
    vc.delegate = self
    return vc
  } ()

  lazy var catalogGalleryViewController: CatalogGalleryViewController = {
    let vc = storyboard!.instantiateViewController(ofType: CatalogGalleryViewController.self)
    vc.view.isHidden = true
    vc.delegate = self
    return vc
  } ()

  lazy var wikiDescriptionViewController: WikiDescriptionViewController = {
    let vc = storyboard!.instantiateViewController(ofType: WikiDescriptionViewController.self)
    vc.view.isHidden = true
    vc.delegate = self
    return vc
  } ()

  lazy var bookmarkViewController: PlacePageBookmarkViewController = {
    let vc = storyboard!.instantiateViewController(ofType: PlacePageBookmarkViewController.self)
    vc.view.isHidden = true
    vc.delegate = self
    return vc
  } ()

  lazy var infoViewController: PlacePageInfoViewController = {
    let vc = storyboard!.instantiateViewController(ofType: PlacePageInfoViewController.self)
    vc.placePageInfoData = placePageData.infoData
    vc.delegate = self
    return vc
  } ()

  lazy var taxiViewController: TaxiViewController = {
    let vc = storyboard!.instantiateViewController(ofType: TaxiViewController.self)
    vc.taxiProvider = placePageData.taxiProvider
    vc.delegate = self
    return vc
  } ()

  lazy var ratingSummaryViewController: RatingSummaryViewController = {
    let vc = storyboard!.instantiateViewController(ofType: RatingSummaryViewController.self)
    vc.view.isHidden = true
    return vc
  } ()

  lazy var addReviewViewController: AddReviewViewController = {
    let vc = storyboard!.instantiateViewController(ofType: AddReviewViewController.self)
    vc.view.isHidden = true
    vc.delegate = self
    return vc
  } ()

  lazy var reviewsViewController: PlacePageReviewsViewController = {
    let vc = storyboard!.instantiateViewController(ofType: PlacePageReviewsViewController.self)
    vc.view.isHidden = true
    vc.delegate = self
    return vc
  } ()

  lazy var buttonsViewController: PlacePageButtonsViewController = {
    let vc = storyboard!.instantiateViewController(ofType: PlacePageButtonsViewController.self)
    vc.buttonsData = placePageData.buttonsData!
    vc.delegate = self
    return vc
  } ()

  lazy var hotelPhotosViewController: HotelPhotosViewController = {
    let vc = storyboard!.instantiateViewController(ofType: HotelPhotosViewController.self)
    vc.view.isHidden = true
    vc.delegate = self
    return vc
  } ()

  lazy var hotelDescriptionViewController: HotelDescriptionViewController = {
    let vc = storyboard!.instantiateViewController(ofType: HotelDescriptionViewController.self)
    vc.view.isHidden = true
    vc.delegate = self
    return vc
  } ()

  lazy var hotelFacilitiesViewController: HotelFacilitiesViewController = {
    let vc = storyboard!.instantiateViewController(ofType: HotelFacilitiesViewController.self)
    vc.view.isHidden = true
    vc.delegate = self
    return vc
  } ()

  lazy var hotelReviewsViewController: HotelReviewsViewController = {
    let vc = storyboard!.instantiateViewController(ofType: HotelReviewsViewController.self)
    vc.view.isHidden = true
    vc.delegate = self
    return vc
  } ()

  lazy var actionBarViewController: ActionBarViewController = {
    let vc = storyboard!.instantiateViewController(ofType: ActionBarViewController.self)
    vc.placePageData = placePageData
    vc.canAddStop = MWMRouter.canAddIntermediatePoint()
    vc.isRoutePlanning = MWMNavigationDashboardManager.shared().state != .hidden
    vc.delegate = self
    return vc
  } ()

  // MARK: - VC Lifecycle

  override func viewDidLoad() {
    super.viewDidLoad()
    if let touchTransparentView = view as? TouchTransparentView {
      touchTransparentView.targetView = scrollView
    }

    addToStack(previewViewController)

    if placePageData.isPromoCatalog {
      addToStack(catalogSingleItemViewController)
      addToStack(catalogGalleryViewController)
      placePageData.loadCatalogPromo { [weak self] in
        guard let self = self else { return }
        guard let catalogPromo = self.placePageData.catalogPromo else {
          if self.placePageData.wikiDescriptionHtml != nil {
            self.wikiDescriptionViewController.view.isHidden = false
          }
          return
        }
        if catalogPromo.promoItems.count == 1 {
          self.catalogSingleItemViewController.promoItem = catalogPromo.promoItems.first!
          self.catalogSingleItemViewController.view.isHidden = false
        } else {
          self.catalogGalleryViewController.promoData = catalogPromo
          self.catalogGalleryViewController.view.isHidden = false
          if self.placePageData.wikiDescriptionHtml != nil {
            self.wikiDescriptionViewController.view.isHidden = false
          }
        }
      }
    }

    addToStack(wikiDescriptionViewController)
    if let wikiDescriptionHtml = placePageData.wikiDescriptionHtml {
      wikiDescriptionViewController.descriptionHtml = wikiDescriptionHtml
      if placePageData.bookmarkData?.bookmarkDescription == nil && !placePageData.isPromoCatalog {
        wikiDescriptionViewController.view.isHidden = false
      }
    }

    addToStack(bookmarkViewController)
    if let bookmarkData = placePageData.bookmarkData {
      bookmarkViewController.bookmarkData = bookmarkData
      bookmarkViewController.view.isHidden = false
    }

    addToStack(hotelPhotosViewController)
    addToStack(hotelDescriptionViewController)
    addToStack(hotelFacilitiesViewController)
    addToStack(hotelReviewsViewController)

    addToStack(infoViewController)

    if placePageData.taxiProvider != .none {
      addToStack(taxiViewController)
    }

    if placePageData.previewData.showUgc {
      addToStack(ratingSummaryViewController)
      addToStack(addReviewViewController)
      addToStack(reviewsViewController)
      placePageData.loadUgc { [weak self] in
        if let self = self, let ugcData =  self.placePageData.ugcData {
          self.previewViewController.updateUgc(ugcData)

          if !ugcData.isTotalRatingEmpty {
            self.ratingSummaryViewController.ugcData = ugcData
            self.ratingSummaryViewController.view.isHidden = false
          }
          if ugcData.isUpdateEmpty {
            self.addReviewViewController.view.isHidden = false
          }
          if !ugcData.isEmpty {
            self.reviewsViewController.ugcData = ugcData
            self.reviewsViewController.view.isHidden = false
          }
          self.updatePreviewOffset()
        }
      }
    }

    if placePageData.previewData.hasBanner,
      let banners = placePageData.previewData.banners {
      BannersCache.cache.get(coreBanners: banners,
                             cacheOnly: false,
                             loadNew: true) { [weak self] (banner, _) in
                              self?.previewViewController.updateBanner(banner)
                              self?.updatePreviewOffset()
      }
    }

    if placePageData.buttonsData != nil {
      addToStack(buttonsViewController)
    }

    placePageData.loadOnlineData { [weak self] in
      if let self = self, let bookingData = self.placePageData.hotelBooking {
        self.previewViewController.updateBooking(bookingData, rooms: self.placePageData.hotelRooms)
        self.stackView.layoutIfNeeded()
        UIView.animate(withDuration: kDefaultAnimationDuration) {
          if !bookingData.photos.isEmpty {
            self.hotelPhotosViewController.photos = bookingData.photos
            self.hotelPhotosViewController.view.isHidden = false
          }
          self.hotelDescriptionViewController.hotelDescription = bookingData.hotelDescription
          self.hotelDescriptionViewController.view.isHidden = false
          if bookingData.facilities.count > 0 {
            self.hotelFacilitiesViewController.facilities = bookingData.facilities
            self.hotelFacilitiesViewController.view.isHidden = false
          }
          if bookingData.reviews.count > 0 {
            self.hotelReviewsViewController.reviewCount = bookingData.scoreCount
            self.hotelReviewsViewController.totalScore = bookingData.score
            self.hotelReviewsViewController.reviews = bookingData.reviews
            self.hotelReviewsViewController.view.isHidden = false
          }
          self.stackView.layoutIfNeeded()
        }
      }
    }

    actionBarViewController.view.translatesAutoresizingMaskIntoConstraints = false
    actionBarContainerView.addSubview(actionBarViewController.view)
    NSLayoutConstraint.activate([
      actionBarViewController.view.leadingAnchor.constraint(equalTo: actionBarContainerView.leadingAnchor),
      actionBarViewController.view.topAnchor.constraint(equalTo: actionBarContainerView.topAnchor),
      actionBarViewController.view.trailingAnchor.constraint(equalTo: actionBarContainerView.trailingAnchor),
      actionBarViewController.view.bottomAnchor.constraint(equalTo: view.safeAreaLayoutGuide.bottomAnchor)
    ])

    MWMLocationManager.add(observer: self)
    if let lastLocation = MWMLocationManager.lastLocation() {
      onLocationUpdate(lastLocation)
    }
    if let lastHeading = MWMLocationManager.lastHeading() {
      onHeadingUpdate(lastHeading)
    }

    let bgView = UIView()
    bgView.styleName = "Background"
    stackView.insertSubview(bgView, at: 0)
    bgView.alignToSuperview()
    scrollView.decelerationRate = .fast
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    if traitCollection.horizontalSizeClass == .compact {
      scrollView.contentInset = UIEdgeInsets(top: scrollView.height, left: 0, bottom: 0, right: 0)
    }
  }

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)
    if !beginDragging {
      updatePreviewOffset()
    }
  }

  // MARK: - Private

  private func calculateSteps() -> [PlacePageState] {
    var steps: [PlacePageState] = []
    steps.append(.closed(-self.scrollView.height))
    let previewFrame = scrollView.convert(previewViewController.view.bounds, from: previewViewController.view)
    steps.append(.preview(previewFrame.maxY - self.scrollView.height))
    if placePageData.isPreviewPlus {
      steps.append(.previewPlus(-self.scrollView.height * 0.55))
    }
    steps.append(.expanded(-self.scrollView.height * 0.3))
    return steps
  }

  private func updatePreviewOffset() {
    self.view.layoutIfNeeded()
    scrollSteps = calculateSteps()
    if traitCollection.horizontalSizeClass != .compact || beginDragging {
      return
    }
    let state = placePageData.isPreviewPlus ? scrollSteps[2] : scrollSteps[1]
    UIView.animate(withDuration: kDefaultAnimationDuration) {
      self.scrollView.contentOffset = CGPoint(x: 0, y: state.offset)
    }
  }

  private func addToStack(_ viewController: UIViewController) {
    addChild(viewController)
    stackView.addArrangedSubview(viewController.view)
    viewController.didMove(toParent: self)
  }
}

// MARK: - PlacePagePreviewViewControllerDelegate

extension PlacePageViewController: PlacePagePreviewViewControllerDelegate {
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

extension PlacePageViewController: PlacePageInfoViewControllerDelegate {
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

extension PlacePageViewController: WikiDescriptionViewControllerDelegate {
  func didPressMore() {
    MWMPlacePageManagerHelper.showPlaceDescription(placePageData.wikiDescriptionHtml)
  }
}

// MARK: - TaxiViewControllerDelegate

extension PlacePageViewController: TaxiViewControllerDelegate {
  func didPressOrder() {
    MWMPlacePageManagerHelper.orderTaxi(placePageData)
  }
}

// MARK: - AddReviewViewControllerDelegate

extension PlacePageViewController: AddReviewViewControllerDelegate {
  func didRate(_ rating: UgcSummaryRatingType) {
    MWMPlacePageManagerHelper.showUGCAddReview(placePageData, rating: rating, from: .placePage)
  }
}

// MARK: - PlacePageReviewsViewControllerDelegate

extension PlacePageViewController: PlacePageReviewsViewControllerDelegate {
  func didPressMoreReviews() {
    let moreReviews = storyboard!.instantiateViewController(ofType: MoreReviewsViewController.self)
    moreReviews.ugcData = placePageData.ugcData
    moreReviews.title = placePageData.previewData.title
    MapViewController.shared()?.navigationController?.pushViewController(moreReviews, animated: true)
  }
}

// MARK: - PlacePageButtonsViewControllerDelegate

extension PlacePageViewController: PlacePageButtonsViewControllerDelegate {
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

extension PlacePageViewController: HotelPhotosViewControllerDelegate {
  func didSelectItemAt(_ index: Int, lastItemIndex: Int) {
    guard let photos = placePageData.hotelBooking?.photos else { return }
    if index == lastItemIndex {
      let galleryController = GalleryViewController.instance(photos: photos)
      galleryController.title = placePageData.previewData.title
      MapViewController.shared()?.navigationController?.pushViewController(galleryController, animated: true)
    } else {
      let currentPhoto = photos[index]
      let view = hotelPhotosViewController.viewForPhoto(currentPhoto)
      let photoVC = PhotosViewController(photos: photos, initialPhoto: currentPhoto, referenceView: view)

      photoVC.referenceViewForPhotoWhenDismissingHandler = { [weak self] in
        self?.hotelPhotosViewController.viewForPhoto($0)
      }
      present(photoVC, animated: true)
    }
  }
}

// MARK: - HotelDescriptionViewControllerDelegate

extension PlacePageViewController: HotelDescriptionViewControllerDelegate {
  func hotelDescriptionDidPressMore() {
    MWMPlacePageManagerHelper.openMoreUrl(placePageData)
  }
}

// MARK: - HotelFacilitiesViewControllerDelegate

extension PlacePageViewController: HotelFacilitiesViewControllerDelegate {
  func facilitiesDidPressMore() {
    MWMPlacePageManagerHelper.showAllFacilities(placePageData)
  }
}

// MARK: - HotelReviewsViewControllerDelegate

extension PlacePageViewController: HotelReviewsViewControllerDelegate {
  func hotelReviewsDidPressMore() {
    MWMPlacePageManagerHelper.openReviewUrl(placePageData)
  }
}

// MARK: - CatalogSingleItemViewControllerDelegate

extension PlacePageViewController: CatalogSingleItemViewControllerDelegate {
  func catalogPromoItemDidPressView() {
    MWMPlacePageManagerHelper.openCatalogSingleItem(placePageData, at: 0)
  }

  func catalogPromoItemDidPressMore() {
    MWMPlacePageManagerHelper.openCatalogSingleItem(placePageData, at: 0)
  }
}

// MARK: - CatalogGalleryViewControllerDelegate

extension PlacePageViewController: CatalogGalleryViewControllerDelegate {
  func promoGalleryDidPressMore() {
    MWMPlacePageManagerHelper.openCatalogMoreItems(placePageData)
  }

  func promoGalleryDidSelectItemAtIndex(_ index: Int) {
    MWMPlacePageManagerHelper.openCatalogSingleItem(placePageData, at: index)
  }
}

// MARK: - PlacePageBookmarkViewControllerDelegate

extension PlacePageViewController: PlacePageBookmarkViewControllerDelegate {
  func bookmarkDidPressEdit() {
    MWMPlacePageManagerHelper.editBookmark()
  }
}

// MARK: - ActionBarViewControllerDelegate

extension PlacePageViewController: ActionBarViewControllerDelegate {
  func actionBarDidPressButton(_ type: ActionBarButtonType) {
    switch type {
    case .booking:
      MWMPlacePageManagerHelper.book(placePageData)
    case .bookingSearch:
      MWMPlacePageManagerHelper.searchSimilar(placePageData)
    case .bookmark:
      if placePageData.bookmarkData != nil {
        MWMPlacePageManagerHelper.removeBookmark(placePageData)
      } else {
        MWMPlacePageManagerHelper.addBookmark(placePageData)
      }
    case .call:
      MWMPlacePageManagerHelper.call(placePageData)
    case .download:
      fatalError()
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
      MWMPlacePageManagerHelper.share(placePageData)
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

// MARK: - UIScrollViewDelegate

extension PlacePageViewController: UIScrollViewDelegate {
  func scrollViewDidScroll(_ scrollView: UIScrollView) {
    if scrollView.contentOffset.y < -scrollView.height + 1 && beginDragging {
      rootViewController.dismissPlacePage()
    }
  }

  func scrollViewWillBeginDragging(_ scrollView: UIScrollView) {
    beginDragging = true
  }

  func scrollViewWillEndDragging(_ scrollView: UIScrollView,
                                 withVelocity velocity: CGPoint,
                                 targetContentOffset: UnsafeMutablePointer<CGPoint>) {
    guard let maxOffset = scrollSteps.last else { return }
    if targetContentOffset.pointee.y > maxOffset.offset {
      previewViewController.adView.state = .detailed
      return
    }

    let targetState = findNextStop(scrollView.contentOffset.y, velocity: velocity.y)
    if targetState.offset > scrollView.contentSize.height - scrollView.contentInset.top {
      previewViewController.adView.state = .detailed
      return
    }
    switch targetState {
    case .closed(_):
      fallthrough
    case .preview(_):
      previewViewController.adView.state = .compact
    case .previewPlus(_):
      fallthrough
    case .expanded(_):
      previewViewController.adView.state = .detailed
    }
    targetContentOffset.pointee = CGPoint(x: 0, y: targetState.offset)
  }

  private func findNearestStop(_ offset: CGFloat) -> PlacePageState{
    var result = scrollSteps[0]
    scrollSteps.suffix(from: 1).forEach { ppState in
      if abs(result.offset - offset) > abs(ppState.offset - offset) {
        result = ppState
      }
    }
    return result
  }

  private func findNextStop(_ offset: CGFloat, velocity: CGFloat) -> PlacePageState {
    if velocity == 0 {
      return findNearestStop(offset)
    }

    var result: PlacePageState
    if velocity < 0 {
      guard let first = scrollSteps.first else { return .closed(-scrollView.height) }
      result = first
      scrollSteps.suffix(from: 1).forEach {
        if offset > $0.offset {
          result = $0
        }
      }
    } else {
      guard let last = scrollSteps.last else { return .closed(-scrollView.height) }
      result = last
      scrollSteps.reversed().suffix(from: 1).forEach {
        if offset < $0.offset {
          result = $0
        }
      }
    }

    return result
  }
}

// MARK: - MWMLocationObserver

extension PlacePageViewController: MWMLocationObserver {
  func onHeadingUpdate(_ heading: CLHeading) {
    if heading.trueHeading < 0 {
      return
    }

    let rad = heading.trueHeading * Double.pi / 180
    previewViewController.updateHeading(CGFloat(rad))
  }

  func onLocationUpdate(_ location: CLLocation) {
    let ppLocation = CLLocation(latitude: placePageData.locationCoordinate.latitude,
                                longitude: placePageData.locationCoordinate.longitude)
    let distance = location.distance(from: ppLocation)
    let distanceFormatter = MKDistanceFormatter()
    distanceFormatter.unitStyle = .abbreviated
    let formattedDistance = distanceFormatter.string(fromDistance: distance)
    previewViewController.updateDistance(formattedDistance)
  }

  func onLocationError(_ locationError: MWMLocationStatus) {
    
  }
}
