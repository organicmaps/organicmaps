final class PlacePageTrackRecordingLayout: IPlacePageLayout {
  private var placePageData: PlacePageData
  private var interactor: PlacePageInteractor
  private let storyboard: UIStoryboard
  weak var presenter: PlacePagePresenterProtocol?

  lazy var bodyViewControllers: [UIViewController] = configureViewControllers()

  var actionBar: ActionBarViewController? {
    actionBarViewController
  }

  var navigationBar: UIViewController? {
    placePageNavigationViewController
  }

  var headerViewControllers: [UIViewController] {
    [headerViewController]
  }

  lazy var headerViewController: PlacePageHeaderViewController = PlacePageHeaderBuilder.build(data: placePageData, delegate: interactor, headerType: .flexible)

  private lazy var placePageNavigationViewController: PlacePageHeaderViewController = PlacePageHeaderBuilder.build(data: placePageData, delegate: interactor, headerType: .fixed)

  private lazy var elevationProfileViewController: ElevationProfileViewController = {
    let state = placePageData.trackData.flatMap(ElevationProfileBuilder.makeTrackRecordingState)
    let viewController = ElevationProfileBuilder.build(state: state, delegate: interactor, presentationStyle: .track)
    viewController.view.isHidden = state == nil
    return viewController
  }()

  private lazy var actionBarViewController: ActionBarViewController = {
    let vc = storyboard.instantiateViewController(ofType: ActionBarViewController.self)
    vc.placePageData = placePageData
    vc.delegate = interactor
    return vc
  }()

  var sectionSpacing: CGFloat { 0.0 }

  init(interactor: PlacePageInteractor, storyboard: UIStoryboard, data: PlacePageData) {
    self.interactor = interactor
    self.storyboard = storyboard
    placePageData = data
  }

  private func configureViewControllers() -> [UIViewController] {
    placePageData.onTrackRecordingProgressUpdate = { [weak self] in
      self?.updateTrackRecordingRelatedSections()
    }
    return [elevationProfileViewController]
  }

  func calculateSteps(inScrollView scrollView: UIScrollView, compact _: Bool) -> [PlacePageState] {
    var steps: [PlacePageState] = []
    let scrollHeight = scrollView.height
    steps.append(.closed(-scrollHeight))
    steps.append(.full)
    return steps
  }
}

private extension PlacePageTrackRecordingLayout {
  func updateTrackRecordingRelatedSections() {
    let preview = placePageData.previewData
    headerViewController.setTitle(preview.title, secondaryTitle: preview.secondaryTitle)

    let state = placePageData.trackData.flatMap(ElevationProfileBuilder.makeTrackRecordingState)
    elevationProfileViewController.view.isHidden = state == nil
    if let state {
      elevationProfileViewController.presenter?.update(with: state)
    }
    presenter?.layoutIfNeeded()
  }
}
