class PlacePageCommonLayout: NSObject, IPlacePageLayout {
  
  private let distanceFormatter = DistanceFormatter.self
  private let altitudeFormatter = AltitudeFormatter.self

  private var placePageData: PlacePageData
  private var interactor: PlacePageInteractor
  private let storyboard: UIStoryboard
  private var lastLocation: CLLocation?
  private weak var editBookmarkInteractor: PlacePageEditBookmarkAndTrackSectionInteractor?

  weak var presenter: PlacePagePresenterProtocol?

  var headerViewControllers: [UIViewController] {
    [headerViewController, previewViewController]
  }

  lazy var bodyViewControllers: [UIViewController] = {
    configureViewControllers()
  }()

  var actionBar: ActionBarViewController? {
    actionBarViewController
  }

  var navigationBar: UIViewController? {
    placePageNavigationViewController
  }
  
  lazy var headerViewController: PlacePageHeaderViewController = {
    PlacePageHeaderBuilder.build(data: placePageData, delegate: interactor, headerType: .flexible)
  }()

  private lazy var previewViewController: PlacePagePreviewViewController = {
    let vc = storyboard.instantiateViewController(ofType: PlacePagePreviewViewController.self)
    vc.placePagePreviewData = placePageData.previewData
    vc.delegate = interactor
    return vc
  }()

  private lazy var osmDescriptionViewController: UIViewController? = {
    guard let osmDescription = placePageData.osmDescription else { return nil }
    return PlacePageExpandableDetailsSectionBuilder.buildOSMDescriptionSection(osmDescription)
  }()

  private lazy var wikiDescriptionViewController: UIViewController? = {
    guard let wikiDescriptionHtml = placePageData.wikiDescriptionHtml else { return nil }
    let showLinkButton = placePageData.infoData?.wikipedia != nil
    return PlacePageExpandableDetailsSectionBuilder.buildWikipediaSection(wikiDescriptionHtml,
                                                                          showLinkButton: showLinkButton,
                                                                          delegate: interactor)
  }()

  private lazy var editBookmarkViewController: PlacePageExpandableDetailsSectionViewController = {
    let vc = PlacePageExpandableDetailsSectionBuilder.buildEditBookmarkAndTrackSection(data: nil, delegate: interactor)
    vc.view.isHidden = true
    editBookmarkInteractor = vc.interactor as? PlacePageEditBookmarkAndTrackSectionInteractor
    return vc
  }()

  private lazy var infoViewController: PlacePageInfoViewController = {
    let vc = storyboard.instantiateViewController(ofType: PlacePageInfoViewController.self)
    vc.placePageInfoData = placePageData.infoData
    vc.delegate = interactor
    return vc
  }()

  private func productsViewController() -> ProductsViewController? {
    let productsManager = FrameworkHelper.self
    guard let configuration = productsManager.getProductsConfiguration() else { return nil }
    let viewModel = ProductsViewModel(manager: productsManager, configuration: configuration)
    return ProductsViewController(viewModel: viewModel)
  }

  private lazy var buttonsViewController: PlacePageOSMContributionViewController = {
    PlacePageOSMContributionViewController(data: placePageData.osmContributionData!, delegate: interactor)
  }()

  private lazy var actionBarViewController: ActionBarViewController = {
    let vc = storyboard.instantiateViewController(ofType: ActionBarViewController.self)
    let navigationManager = MWMNavigationDashboardManager.shared()
    vc.placePageData = placePageData
    vc.isRoutePlanning = navigationManager.state != .closed
    vc.canAddStop = MWMRouter.canAddIntermediatePoint()
    vc.canReplaceStop = navigationManager.selectedRoutePoint != nil
    vc.canRouteToAndFrom = !navigationManager.shouldAppendNewPoints && navigationManager.selectedRoutePoint == nil
    vc.delegate = interactor
    return vc
  }()

  private lazy var placePageNavigationViewController: PlacePageHeaderViewController = {
    return PlacePageHeaderBuilder.build(data: placePageData, delegate: interactor, headerType: .fixed)
  }()

  init(interactor: PlacePageInteractor, storyboard: UIStoryboard, data: PlacePageData) {
    self.interactor = interactor
    self.storyboard = storyboard
    self.placePageData = data
  }

  private func configureViewControllers() -> [UIViewController] {
    var viewControllers = [UIViewController]()

    viewControllers.append(editBookmarkViewController)
    if let bookmarkData = placePageData.bookmarkData {
      editBookmarkViewController.view.isHidden = false
      editBookmarkInteractor?.data = .bookmark(bookmarkData)
    }

    if let osmDescriptionViewController {
      viewControllers.append(osmDescriptionViewController)
    }

    if let wikiDescriptionViewController {
      viewControllers.append(wikiDescriptionViewController)
    }

    if placePageData.infoData != nil {
      viewControllers.append(infoViewController)
    }

    if let productsViewController = productsViewController() {
      viewControllers.append(productsViewController)
    }

    if placePageData.osmContributionData != nil {
      viewControllers.append(buttonsViewController)
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
      if let buttonsData = self.placePageData.osmContributionData {
        self.buttonsViewController.buttonsData = buttonsData
      }
      switch self.placePageData.mapNodeAttributes!.nodeStatus {
      case .onDisk, .onDiskOutOfDate, .undefined, .inQueue:
        self.actionBarViewController.resetButtons()
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
    preview.layoutIfNeeded()
    let previewFrame = scrollView.convert(preview.bounds, from: preview)
    steps.append(.preview(previewFrame.maxY - scrollHeight))
    if !compact, placePageData.isPreviewPlus {
      steps.append(.previewPlus(-scrollHeight * 0.55))
    }
    steps.append(.full)
    return steps
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
      let altString = "▲ \(altitudeFormatter.altitudeString(fromMeters: location.altitude))"
      if location.speed > 0 && location.timestamp.timeIntervalSinceNow >= -2 {
        let speedMeasure = Measure.init(asSpeed: location.speed)
        let speedString = "\(LocationManager.speedSymbolFor(location.speed))\(speedMeasure.valueAsString) \(speedMeasure.unit)"
        previewViewController.updateSpeedAndAltitude("\(altString)  \(speedString)")
      } else {
        previewViewController.updateSpeedAndAltitude(altString)
      }
    } else {
      let ppLocation = CLLocation(latitude: placePageData.locationCoordinate.latitude,
                                  longitude: placePageData.locationCoordinate.longitude)
      let distance = location.distance(from: ppLocation)
      let formattedDistance = distanceFormatter.distanceString(fromMeters: distance)
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
