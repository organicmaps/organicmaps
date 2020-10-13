class PlacePageCommonLayout: NSObject, IPlacePageLayout {
  private lazy var distanceFormatter: MKDistanceFormatter = {
    let formatter =  MKDistanceFormatter()
    formatter.unitStyle = .abbreviated
    formatter.units = Settings.measurementUnits() == .imperial ? .imperial : .metric
    return formatter
  }()

  private lazy var unitsFormatter: MeasurementFormatter = {
    let formatter = MeasurementFormatter()
    formatter.unitOptions = [.providedUnit]
    return formatter
  }()

  private var placePageData: PlacePageData
  private var interactor: PlacePageInteractor
  private let storyboard: UIStoryboard
  weak var presenter: PlacePagePresenterProtocol?

  fileprivate var lastLocation: CLLocation?

  lazy var viewControllers: [UIViewController] = {
    return configureViewControllers()
  }()

  var actionBar: ActionBarViewController? {
    return actionBarViewController
  }

  var navigationBar: UIViewController? {
    return placePageNavigationViewController
  }

  var adState: AdBannerState = .unset {
    didSet {
      previewViewController.adView.state = self.adState
    }
  }

  lazy var previewViewController: PlacePagePreviewViewController = {
    let vc = storyboard.instantiateViewController(ofType: PlacePagePreviewViewController.self)
    vc.placePagePreviewData = placePageData.previewData
    vc.delegate = interactor
    return vc
  } ()
  
  lazy var catalogSingleItemViewController: CatalogSingleItemViewController = {
    let vc = storyboard.instantiateViewController(ofType: CatalogSingleItemViewController.self)
    vc.view.isHidden = true
    vc.delegate = interactor
    return vc
  } ()
  
  lazy var catalogGalleryViewController: CatalogGalleryViewController = {
    let vc = storyboard.instantiateViewController(ofType: CatalogGalleryViewController.self)
    vc.view.isHidden = true
    vc.delegate = interactor
    return vc
  } ()
  
  lazy var wikiDescriptionViewController: WikiDescriptionViewController = {
    let vc = storyboard.instantiateViewController(ofType: WikiDescriptionViewController.self)
    vc.view.isHidden = true
    vc.delegate = interactor
    return vc
  } ()

  lazy var descriptionDividerViewController: PlacePageDividerViewController = {
    let vc = storyboard.instantiateViewController(ofType: PlacePageDividerViewController.self)
    vc.view.isHidden = true
    if let bookmarkData = placePageData.bookmarkData {
      let group = BookmarkGroup(categoryId: bookmarkData.bookmarkGroupId, bookmarksManager: BookmarksManager.shared())
      vc.isAuthorIconHidden = !group.isLonelyPlanet
    }
    vc.titleText = L("placepage_place_description").uppercased()
    return vc
  } ()

  lazy var keyInformationDividerViewController: PlacePageDividerViewController = {
    let vc = storyboard.instantiateViewController(ofType: PlacePageDividerViewController.self)
    vc.view.isHidden = true
    vc.titleText = L("key_information_title").uppercased()
    return vc
  } ()

  lazy var bookmarkViewController: PlacePageBookmarkViewController = {
    let vc = storyboard.instantiateViewController(ofType: PlacePageBookmarkViewController.self)
    vc.view.isHidden = true
    vc.delegate = interactor
    return vc
  } ()
  
  lazy var infoViewController: PlacePageInfoViewController = {
    let vc = storyboard.instantiateViewController(ofType: PlacePageInfoViewController.self)
    vc.placePageInfoData = placePageData.infoData
    vc.delegate = interactor
    return vc
  } ()
  
  lazy var taxiViewController: TaxiViewController = {
    let vc = storyboard.instantiateViewController(ofType: TaxiViewController.self)
    vc.taxiProvider = placePageData.taxiProvider
    vc.delegate = interactor
    return vc
  } ()
  
  lazy var ratingSummaryViewController: RatingSummaryViewController = {
    let vc = storyboard.instantiateViewController(ofType: RatingSummaryViewController.self)
    vc.view.isHidden = true
    return vc
  } ()
  
  lazy var addReviewViewController: AddReviewViewController = {
    let vc = storyboard.instantiateViewController(ofType: AddReviewViewController.self)
    vc.view.isHidden = true
    vc.delegate = interactor
    return vc
  } ()
  
  lazy var reviewsViewController: PlacePageReviewsViewController = {
    let vc = storyboard.instantiateViewController(ofType: PlacePageReviewsViewController.self)
    vc.view.isHidden = true
    vc.delegate = interactor
    return vc
  } ()
  
  lazy var buttonsViewController: PlacePageButtonsViewController = {
    let vc = storyboard.instantiateViewController(ofType: PlacePageButtonsViewController.self)
    vc.buttonsData = placePageData.buttonsData!
    vc.buttonsEnabled = placePageData.mapNodeAttributes?.nodeStatus == .onDisk
    vc.delegate = interactor
    return vc
  } ()
  
  lazy var hotelPhotosViewController: HotelPhotosViewController = {
    let vc = storyboard.instantiateViewController(ofType: HotelPhotosViewController.self)
    vc.view.isHidden = true
    vc.delegate = interactor
    return vc
  } ()
  
  lazy var hotelDescriptionViewController: HotelDescriptionViewController = {
    let vc = storyboard.instantiateViewController(ofType: HotelDescriptionViewController.self)
    vc.view.isHidden = true
    vc.delegate = interactor
    return vc
  } ()
  
  lazy var hotelFacilitiesViewController: HotelFacilitiesViewController = {
    let vc = storyboard.instantiateViewController(ofType: HotelFacilitiesViewController.self)
    vc.view.isHidden = true
    vc.delegate = interactor
    return vc
  } ()
  
  lazy var hotelReviewsViewController: HotelReviewsViewController = {
    let vc = storyboard.instantiateViewController(ofType: HotelReviewsViewController.self)
    vc.view.isHidden = true
    vc.delegate = interactor
    return vc
  } ()
  
  lazy var actionBarViewController: ActionBarViewController = {
    let vc = storyboard.instantiateViewController(ofType: ActionBarViewController.self)
    vc.placePageData = placePageData
    vc.canAddStop = MWMRouter.canAddIntermediatePoint()
    vc.isRoutePlanning = MWMNavigationDashboardManager.shared().state != .hidden
    vc.delegate = interactor
    return vc
  } ()

  lazy var header: PlacePageHeaderViewController? = {
    return PlacePageHeaderBuilder.build(data: placePageData.previewData, delegate: interactor, headerType: .flexible)
  } ()

  lazy var placePageNavigationViewController: PlacePageHeaderViewController = {
    return PlacePageHeaderBuilder.build(data: placePageData.previewData, delegate: interactor, headerType: .fixed)
  } ()
  
  init(interactor: PlacePageInteractor, storyboard: UIStoryboard, data: PlacePageData) {
    self.interactor = interactor
    self.storyboard = storyboard
    self.placePageData = data
  }
  
  private func configureViewControllers() -> [UIViewController] {
    var viewControllers = [UIViewController]()
    viewControllers.append(previewViewController)
    if placePageData.isPromoCatalog {
      viewControllers.append(catalogSingleItemViewController)
      viewControllers.append(catalogGalleryViewController)
      placePageData.loadCatalogPromo(completion: onLoadCatalogPromo)
    }

    viewControllers.append(descriptionDividerViewController)
    viewControllers.append(wikiDescriptionViewController)
    if let wikiDescriptionHtml = placePageData.wikiDescriptionHtml {
      wikiDescriptionViewController.descriptionHtml = wikiDescriptionHtml
      if placePageData.bookmarkData?.bookmarkDescription == nil && !placePageData.isPromoCatalog {
        wikiDescriptionViewController.view.isHidden = false
        descriptionDividerViewController.view.isHidden = false
      }
    }

    viewControllers.append(bookmarkViewController)
    if let bookmarkData = placePageData.bookmarkData {
      bookmarkViewController.bookmarkData = bookmarkData
      bookmarkViewController.view.isHidden = false
      if let description = bookmarkData.bookmarkDescription, description.isEmpty == false {
        descriptionDividerViewController.view.isHidden = false
      }
    }

    viewControllers.append(hotelPhotosViewController)
    viewControllers.append(hotelDescriptionViewController)
    viewControllers.append(hotelFacilitiesViewController)
    viewControllers.append(hotelReviewsViewController)

    if placePageData.infoData != nil {
      viewControllers.append(keyInformationDividerViewController)
      keyInformationDividerViewController.view.isHidden = false
      viewControllers.append(infoViewController)
    }

    if placePageData.taxiProvider != .none &&
      !LocationManager.isLocationProhibited() &&
      FrameworkHelper.isNetworkConnected() {
        viewControllers.append(taxiViewController)
    }

    if placePageData.previewData.showUgc {
      viewControllers.append(ratingSummaryViewController)
      viewControllers.append(addReviewViewController)
      viewControllers.append(reviewsViewController)
      placePageData.loadUgc(completion: onLoadUgc)
    }

    if placePageData.previewData.hasBanner,
      let banners = placePageData.previewData.banners {
      BannersCache.cache.get(coreBanners: banners, cacheOnly: false, loadNew: true, completion: onGetBanner)
    }

    if placePageData.buttonsData != nil {
      viewControllers.append(buttonsViewController)
    }
    
    placePageData.loadOnlineData(completion: onLoadOnlineData)
    placePageData.onBookmarkStatusUpdate = { [weak self] in
      guard let self = self else { return }
      if self.placePageData.bookmarkData == nil {
        self.actionBarViewController.resetButtons()
      }
      self.previewViewController.placePagePreviewData = self.placePageData.previewData
      self.updateBookmarkRelatedSections()
    }
    placePageData.onUgcStatusUpdate = { [weak self] in
      self?.onLoadUgc()
    }

    LocationManager.add(observer: self)
    if let lastLocation = LocationManager.lastLocation() {
      onLocationUpdate(lastLocation)
      self.lastLocation = lastLocation
    }
    if let lastHeading = LocationManager.lastHeading() {
      onHeadingUpdate(lastHeading)
    }

    placePageData.onMapNodeStatusUpdate = { [weak self] in
      guard let self = self else { return }
      self.actionBarViewController.updateDownloadButtonState(self.placePageData.mapNodeAttributes!.nodeStatus)
      switch self.placePageData.mapNodeAttributes!.nodeStatus {
      case .onDisk, .onDiskOutOfDate, .undefined:
        self.actionBarViewController.resetButtons()
        if self.placePageData.buttonsData != nil {
          self.buttonsViewController.buttonsEnabled = true
        }
      default:
        break
      }
    }
    placePageData.onMapNodeProgressUpdate = { [weak self] (downloadedBytes, totalBytes) in
      guard let self = self, let downloadButton = self.actionBarViewController.downloadButton else { return }
      downloadButton.mapDownloadProgress?.progress = CGFloat(downloadedBytes) / CGFloat(totalBytes)
    }

    return viewControllers
  }

  func calculateSteps(inScrollView scrollView: UIScrollView, compact: Bool) -> [PlacePageState] {
    var steps: [PlacePageState] = []
    let scrollHeight = scrollView.height
    steps.append(.closed(-scrollHeight))
    guard let preview = previewViewController.view else {
      return steps
    }
    let previewFrame = scrollView.convert(preview.bounds, from: preview)
    steps.append(.preview(previewFrame.maxY - scrollHeight))
    if !compact {
      if placePageData.isPreviewPlus {
        steps.append(.previewPlus(-scrollHeight * 0.55))
      }
      steps.append(.expanded(-scrollHeight * 0.3))
    }
    steps.append(.full(0))
    return steps
  }
}


// MARK: - PlacePageData async callbacks for loaders

extension PlacePageCommonLayout {
  func onLoadOnlineData() {
    if let bookingData = self.placePageData.hotelBooking {
      previewViewController.updateBooking(bookingData, rooms: self.placePageData.hotelRooms)
      presenter?.layoutIfNeeded()
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
        self.presenter?.layoutIfNeeded()
      }
    }
  }

  func onLoadUgc() {
    if let ugcData = self.placePageData.ugcData {
      previewViewController.updateUgc(ugcData)

      if !ugcData.isTotalRatingEmpty {
        ratingSummaryViewController.ugcData = ugcData
        ratingSummaryViewController.view.isHidden = false
      }
      addReviewViewController.view.isHidden = !ugcData.isUpdateEmpty
      if !ugcData.isEmpty || !ugcData.isUpdateEmpty {
        reviewsViewController.ugcData = ugcData
        reviewsViewController.view.isHidden = false
      }
      presenter?.updatePreviewOffset()
    }
  }

  func onLoadCatalogPromo() {
    guard let catalogPromo = self.placePageData.catalogPromo, catalogPromo.promoItems.count > 0 else {
      if self.placePageData.wikiDescriptionHtml != nil {
        wikiDescriptionViewController.view.isHidden = false
      }
      return
    }

    let statPlacement: String
    switch (self.placePageData.sponsoredType)
    {
    case .promoCatalogCity:
      statPlacement = kStatPlacePageToponims;
    case .promoCatalogSightseeings:
      statPlacement = kStatPlacePageSightSeeing;
    case .promoCatalogOutdoor:
      statPlacement = kStatPlacePageOutdoor;
    default:
      statPlacement =  kStatUnknownError;
    }
    Statistics.logEvent(kStatPlacepageSponsoredShow, withParameters: [kStatProvider: kStatMapsmeGuides,
                                                                      kStatState: kStatOnline,
                                                                      kStatCount: catalogPromo.promoItems.count,
                                                                      kStatPlacement: statPlacement])
    updateCatalogPromoVisibility()
  }

  func updateCatalogPromoVisibility() {
    guard let catalogPromo = self.placePageData.catalogPromo, catalogPromo.promoItems.count > 0 else {
      catalogSingleItemViewController.view.isHidden = true
      catalogGalleryViewController.view.isHidden = true
      return
    }

    let isBookmark = placePageData.bookmarkData != nil
    if isBookmark {
      catalogSingleItemViewController.view.isHidden = true
      catalogGalleryViewController.view.isHidden = true
      return
    }

    if catalogPromo.promoItems.count == 1 {
      catalogSingleItemViewController.promoItem = catalogPromo.promoItems.first!
      catalogSingleItemViewController.view.isHidden = false
    } else {
      catalogGalleryViewController.promoData = catalogPromo
      catalogGalleryViewController.view.isHidden = false
      if self.placePageData.wikiDescriptionHtml != nil {
        wikiDescriptionViewController.view.isHidden = false
      }
    }
  }

  func onGetBanner(banner: MWMBanner, loadNew: Bool) -> Void {
    previewViewController.updateBanner(banner)
    presenter?.updatePreviewOffset()
  }

  func updateBookmarkRelatedSections() {
    var isBookmark = false
    if let bookmarkData = placePageData.bookmarkData {
      bookmarkViewController.bookmarkData = bookmarkData
      isBookmark = true
    }
    self.presenter?.layoutIfNeeded()
    UIView.animate(withDuration: kDefaultAnimationDuration) { [unowned self] in
      self.bookmarkViewController.view.isHidden = !isBookmark
      if (isBookmark) {
        self.catalogGalleryViewController.view.isHidden = true
        self.catalogSingleItemViewController.view.isHidden = true
      } else {
        self.updateCatalogPromoVisibility()
      }
      self.presenter?.layoutIfNeeded()
    }
  }
}

// MARK: - MWMLocationObserver

extension PlacePageCommonLayout: MWMLocationObserver {
  func onHeadingUpdate(_ heading: CLHeading) {
    if !placePageData.isMyPosition {
      updateHeading(heading.trueHeading)
    }
  }

  func onLocationUpdate(_ location: CLLocation) {
    if placePageData.isMyPosition {
      let imperial = Settings.measurementUnits() == .imperial
      let alt = imperial ? location.altitude / 0.3048 : location.altitude
      let altMeasurement = Measurement(value: alt.rounded(), unit: imperial ? UnitLength.feet : UnitLength.meters)
      let altString = "▲ \(unitsFormatter.string(from: altMeasurement))"

      if location.speed > 0 && location.timestamp.timeIntervalSinceNow >= -2 {
        let speed = imperial ? location.speed * 2.237 : location.speed * 3.6
        let speedMeasurement = Measurement(value: speed.rounded(), unit: imperial ? UnitSpeed.milesPerHour: UnitSpeed.kilometersPerHour)
        let speedString = "\(LocationManager.speedSymbolFor(location.speed))\(unitsFormatter.string(from: speedMeasurement))"
        previewViewController.updateSpeedAndAltitude("\(altString)  \(speedString)")
      } else {
        previewViewController.updateSpeedAndAltitude(altString)
      }
    } else {
      let ppLocation = CLLocation(latitude: placePageData.locationCoordinate.latitude,
                                  longitude: placePageData.locationCoordinate.longitude)
      let distance = location.distance(from: ppLocation)
      let formattedDistance = distanceFormatter.string(fromDistance: distance)
      previewViewController.updateDistance(formattedDistance)

      lastLocation = location
    }
  }

  func onLocationError(_ locationError: MWMLocationStatus) {

  }

  private func updateHeading(_ heading: CLLocationDirection) {
    guard let location = lastLocation, heading > 0 else {
      return
    }

    let rad = heading * Double.pi / 180
    let angle = GeoUtil.angle(atPoint: location.coordinate, toPoint: placePageData.locationCoordinate)
    previewViewController.updateHeading(CGFloat(angle) + CGFloat(rad))
  }
}
