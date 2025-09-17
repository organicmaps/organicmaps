class PlacePageTrackLayout: IPlacePageLayout {
  private var placePageData: PlacePageData
  private var trackData: PlacePageTrackData
  private var interactor: PlacePageInteractor
  private let storyboard: UIStoryboard
  private weak var editTrackInteractor: PlacePageEditBookmarkAndTrackSectionInteractor?

  weak var presenter: PlacePagePresenterProtocol?

  lazy var bodyViewControllers: [UIViewController] = {
    return configureViewControllers()
  }()

  var actionBar: ActionBarViewController? {
    actionBarViewController
  }

  var navigationBar: UIViewController? {
    placePageNavigationViewController
  }

  var headerViewControllers: [UIViewController] {
    [headerViewController, previewViewController]
  }

  lazy var headerViewController: PlacePageHeaderViewController = {
    PlacePageHeaderBuilder.build(data: placePageData, delegate: interactor, headerType: .flexible)
  }()

  private lazy var previewViewController: PlacePagePreviewViewController = {
    let vc = storyboard.instantiateViewController(ofType: PlacePagePreviewViewController.self)
    vc.placePagePreviewData = placePageData.previewData
    return vc
  }()

  private lazy var placePageNavigationViewController: PlacePageHeaderViewController = {
    return PlacePageHeaderBuilder.build(data: placePageData, delegate: interactor, headerType: .fixed)
  }()

  private lazy var editTrackViewController: PlacePageExpandableDetailsSectionViewController = {
    let vc = PlacePageExpandableDetailsSectionBuilder.buildEditBookmarkAndTrackSection(data: nil, delegate: interactor)
    vc.view.isHidden = true
    editTrackInteractor = vc.interactor as? PlacePageEditBookmarkAndTrackSectionInteractor
    return vc
  }()

  lazy var elevationMapViewController: ElevationProfileViewController? = {
    guard trackData.trackInfo.hasElevationInfo, trackData.elevationProfileData != nil else {
      return nil
    }
    return ElevationProfileBuilder.build(trackData: trackData, delegate: interactor)
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

  init(interactor: PlacePageInteractor, storyboard: UIStoryboard, data: PlacePageData) {
    self.interactor = interactor
    self.storyboard = storyboard
    self.placePageData = data
    guard let trackData = data.trackData else {
      fatalError("PlacePageData must contain trackData for the PlacePageTrackLayout")
    }
    self.trackData = trackData
  }

  private func configureViewControllers() -> [UIViewController] {
    var viewControllers = [UIViewController]()

    viewControllers.append(editTrackViewController)
    if let trackData = placePageData.trackData {
      editTrackViewController.view.isHidden = false
      editTrackInteractor?.data = .track(trackData)
    }

    if let elevationMapViewController {
      viewControllers.append(elevationMapViewController)
    }

    return viewControllers
  }

  func calculateSteps(inScrollView scrollView: UIScrollView, compact: Bool) -> [PlacePageState] {
    var steps: [PlacePageState] = []
    let scrollHeight = scrollView.height
    steps.append(.closed(-scrollHeight))
    steps.append(.full)
    return steps
  }
}
