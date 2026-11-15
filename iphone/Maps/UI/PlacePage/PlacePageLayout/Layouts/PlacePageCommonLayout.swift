class PlacePageCommonLayout: NSObject, IPlacePageLayout {
  private let distanceFormatter = DistanceFormatter.self
  private let altitudeFormatter = AltitudeFormatter.self

  private var placePageData: PlacePageData
  private var interactor: PlacePageInteractor
  private let storyboard: UIStoryboard
  private let headingProvider: () -> CLHeading?
  private var targetAngle: Float?
  private weak var editBookmarkInteractor: PlacePageEditBookmarkAndTrackSectionInteractor?

  weak var presenter: PlacePagePresenterProtocol?

  var headerViewControllers: [UIViewController] {
    [headerViewController, previewViewController]
  }

  lazy var bodyViewControllers: [UIViewController] = configureViewControllers()

  var actionBar: ActionBarViewController? {
    actionBarViewController
  }

  var navigationBar: UIViewController? {
    placePageNavigationViewController
  }

  lazy var headerViewController: PlacePageHeaderViewController = PlacePageHeaderBuilder.build(data: placePageData, delegate: interactor, headerType: .flexible)

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

  private lazy var buttonsViewController: PlacePageOSMContributionViewController = .init(data: placePageData.osmContributionData!, delegate: interactor)

  private lazy var actionBarViewController: ActionBarViewController = {
    let vc = storyboard.instantiateViewController(ofType: ActionBarViewController.self)
    let navigationManager = MWMNavigationDashboardManager.shared()
    vc.placePageData = placePageData
    vc.canAddStop = MWMRouter.canAddIntermediatePoint()
    vc.canReplaceStop = navigationManager.selectedRoutePoint != nil
    vc.shouldAppendNewPoints = navigationManager.shouldAppendNewPoints
    vc.delegate = interactor
    return vc
  }()

  private lazy var placePageNavigationViewController: PlacePageHeaderViewController = PlacePageHeaderBuilder.build(data: placePageData, delegate: interactor, headerType: .fixed)

  init(interactor: PlacePageInteractor, storyboard: UIStoryboard, data: PlacePageData,
       headingProvider: @escaping () -> CLHeading? = LocationManager.lastHeading) {
    self.interactor = interactor
    self.storyboard = storyboard
    placePageData = data
    self.headingProvider = headingProvider
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
    }

    placePageData.onMapNodeStatusUpdate = { [weak self] in
      guard let self = self else { return }
      if let buttonsData = self.placePageData.osmContributionData {
        self.buttonsViewController.buttonsData = buttonsData
      }
      self.actionBarViewController.updateMapNodeStatus()
    }
    placePageData.onMapNodeProgressUpdate = { [weak self] downloadedBytes, totalBytes in
      guard let self = self else { return }
      self.actionBarViewController.downloadButton?.mapDownloadProgress?.progress =
        CGFloat(AppConstants.maxProgress) * CGFloat(downloadedBytes) / CGFloat(totalBytes)
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
    updateHeading(using: heading)
  }

  func onLocationUpdate(_ location: CLLocation) {
    if placePageData.isMyPosition {
      let altString = "▲ \(altitudeFormatter.altitudeString(fromMeters: location.altitude))"
      if location.speed > 0, location.timestamp.timeIntervalSinceNow >= -2 {
        let speedMeasure = Measure(asSpeed: location.speed)
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

      let angle = GeoUtil.angle(atPoint: location.coordinate, toPoint: placePageData.locationCoordinate)
      targetAngle = angle
      previewViewController.updateBearing(Self.bearingDegrees(from: angle))
      updateHeading(using: headingProvider())
    }
  }

  func onLocationError(_: MWMLocationStatus) {}

  /// Converts GeoUtil's counterclockwise radians from east to rounded clockwise degrees from true north (0..<360).
  /// Adding a full turn keeps the remainder nonnegative.
  static func bearingDegrees(from angle: Float) -> Int {
    let bearing = (90 - Double(angle) * 180 / .pi + 360)
      .truncatingRemainder(dividingBy: 360)
    return Int(bearing.rounded()) % 360
  }

  private func updateHeading(using heading: CLHeading?) {
    guard let angle = targetAngle else {
      return
    }
    guard let heading, heading.trueHeading >= 0 else {
      previewViewController.updateHeading(nil)
      return
    }
    let rad = heading.trueHeading * .pi / 180
    previewViewController.updateHeading(CGFloat(angle) + CGFloat(rad))
  }
}
