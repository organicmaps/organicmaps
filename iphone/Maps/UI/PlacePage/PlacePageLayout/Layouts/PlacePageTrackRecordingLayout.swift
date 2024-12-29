final class PlacePageTrackRecordingLayout: IPlacePageLayout {
  private var placePageData: PlacePageData
  private var interactor: PlacePageInteractor
  private let storyboard: UIStoryboard
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

  lazy var headerViewControllers: [UIViewController] = {
    [headerViewController]
  }()

  lazy var headerViewController: PlacePageHeaderViewController = {
    return PlacePageHeaderBuilder.build(data: placePageData.previewData, delegate: interactor, headerType: .flexible)
  }()

  lazy var placePageNavigationViewController: PlacePageHeaderViewController = {
    return PlacePageHeaderBuilder.build(data: placePageData.previewData, delegate: interactor, headerType: .fixed)
  }()

  lazy var editTrackViewController: PlacePageEditBookmarkOrTrackViewController = {
    let vc = storyboard.instantiateViewController(ofType: PlacePageEditBookmarkOrTrackViewController.self)
    vc.view.isHidden = true
    vc.delegate = interactor
    return vc
  }()

  lazy var elevationMapViewController: ElevationProfileViewController? = {
    guard let trackData = placePageData.trackData else {
      return nil
    }
    return ElevationProfileBuilder.build(trackInfo: trackData.trackInfo,
                                         elevationProfileData: trackData.elevationProfileData,
                                         delegate: interactor)
  }()

  lazy var actionBarViewController: ActionBarViewController = {
    let vc = storyboard.instantiateViewController(ofType: ActionBarViewController.self)
    vc.placePageData = placePageData
    vc.canAddStop = MWMRouter.canAddIntermediatePoint()
    vc.isRoutePlanning = MWMNavigationDashboardManager.shared().state != .hidden
    vc.delegate = interactor
    return vc
  }()

  init(interactor: PlacePageInteractor, storyboard: UIStoryboard, data: PlacePageData) {
    self.interactor = interactor
    self.storyboard = storyboard
    self.placePageData = data
  }

  private func configureViewControllers() -> [UIViewController] {
    var viewControllers = [UIViewController]()

    if let elevationMapViewController {
      viewControllers.append(elevationMapViewController)
    }

    return viewControllers
  }

  func calculateSteps(inScrollView scrollView: UIScrollView, compact: Bool) -> [PlacePageState] {
    var steps: [PlacePageState] = []
    let scrollHeight = scrollView.height
    steps.append(.closed(-scrollHeight))
    steps.append(.full(0))
    return steps
  }
}

private extension PlacePageTrackRecordingLayout {
  func updateTrackRelatedSections() {
//    guard let trackData = placePageData.trackData else {
//      presenter?.closeAnimated()
//      return
//    }
//    editTrackViewController.data = .track(trackData)
//    let previewData = placePageData.previewData
//    if let headerViewController = headerViewControllers.compactMap({ $0 as? PlacePageHeaderViewController }).first {
//      headerViewController.setTitle(previewData.title, secondaryTitle: previewData.secondaryTitle)
//      placePageNavigationViewController.setTitle(previewData.title, secondaryTitle: previewData.secondaryTitle)
//    }
//    if let previewViewController = headerViewControllers.compactMap({ $0 as? PlacePagePreviewViewController }).first {
//      previewViewController.placePagePreviewData = previewData
//      previewViewController.updateViews()
//    }
//    presenter?.layoutIfNeeded()
  }
}
