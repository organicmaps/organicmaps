class PlacePageElevationLayout: IPlacePageLayout {
  private var placePageData: PlacePageData
  private var interactor: PlacePageInteractor
  private let storyboard: UIStoryboard
  weak var presenter: PlacePagePresenterProtocol?

  lazy var bodyViewControllers: [UIViewController] = {
    return configureViewControllers()
  }()

  var actionBar: ActionBarViewController? = nil

  var navigationBar: UIViewController? {
    return placePageNavigationViewController
  }

  lazy var headerViewControllers: [UIViewController] = {
    return [PlacePageHeaderBuilder.build(data: placePageData.previewData, delegate: interactor, headerType: .flexible)]
  } ()

  lazy var placePageNavigationViewController: PlacePageHeaderViewController = {
    return PlacePageHeaderBuilder.build(data: placePageData.previewData, delegate: interactor, headerType: .fixed)
  } ()

  lazy var bookmarkViewController: PlacePageBookmarkViewController = {
    let vc = storyboard.instantiateViewController(ofType: PlacePageBookmarkViewController.self)
    vc.view.isHidden = true
    vc.delegate = interactor
    return vc
  } ()

  func trackStatisticsViewController(statistics: TrackStatistics) -> TrackStatisticsViewController {
    let vc = TrackStatisticsBuilder.build(statistics: statistics)
    return vc
  }

  lazy var elevationMapViewController: ElevationProfileViewController? = {
    guard let elevationProfileData = placePageData.elevationProfileData else {
      return nil
    }
    return ElevationProfileBuilder.build(elevationProfileData: elevationProfileData, delegate: interactor)
  } ()

  init(interactor: PlacePageInteractor, storyboard: UIStoryboard, data: PlacePageData) {
    self.interactor = interactor
    self.storyboard = storyboard
    self.placePageData = data
  }

  private func configureViewControllers() -> [UIViewController] {
    var viewControllers = [UIViewController]()

    viewControllers.append(bookmarkViewController)
    if let bookmarkData = placePageData.bookmarkData {
      bookmarkViewController.bookmarkData = bookmarkData
      bookmarkViewController.view.isHidden = false
    }

    guard let trackStatistics = placePageData.trackStatistics else {
      let message = "Track statistics should not be nil"
      LOG(.critical, message)
      fatalError(message)
    }
    viewControllers.append(trackStatisticsViewController(statistics: trackStatistics))
    if let elevationMapViewController {
      viewControllers.append(elevationMapViewController)
    }

    return viewControllers
  }

  func calculateSteps(inScrollView scrollView: UIScrollView, compact: Bool) -> [PlacePageState] {
    var steps: [PlacePageState] = []
    let scrollHeight = scrollView.height
    steps.append(.closed(-scrollHeight))
    guard let elevationMapViewController else {
      steps.append(.full(0))
      return steps
    }
    let previewHeight = elevationMapViewController.getPreviewHeight()
    guard let previewView = elevationMapViewController.view else {
        return steps
    }
    let previewFrame = scrollView.convert(previewView.bounds, from: previewView)
    steps.append(.preview(previewFrame.maxY - scrollHeight - previewHeight))
    steps.append(.expanded(previewFrame.maxY - scrollHeight))
    steps.append(.full(0))
    return steps
  }
}
