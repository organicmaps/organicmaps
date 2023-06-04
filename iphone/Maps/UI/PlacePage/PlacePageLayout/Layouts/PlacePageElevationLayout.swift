class PlacePageElevationLayout: IPlacePageLayout {
  private var placePageData: PlacePageData
  private var interactor: PlacePageInteractor
  private let storyboard: UIStoryboard
  weak var presenter: PlacePagePresenterProtocol?

  lazy var viewControllers: [UIViewController] = {
    return configureViewControllers()
  }()

  var actionBar: ActionBarViewController? = nil

  var navigationBar: UIViewController? {
    return placePageNavigationViewController
  }

  lazy var header: PlacePageHeaderViewController? = {
    return PlacePageHeaderBuilder.build(data: placePageData.previewData, delegate: interactor, headerType: .flexible)
  } ()

  lazy var placePageNavigationViewController: PlacePageHeaderViewController = {
    return PlacePageHeaderBuilder.build(data: placePageData.previewData, delegate: interactor, headerType: .fixed)
  } ()

  lazy var elevationMapViewController: ElevationProfileViewController = {
    let vc = ElevationProfileBuilder.build(data: placePageData, delegate: interactor)
    return vc
  } ()

  init(interactor: PlacePageInteractor, storyboard: UIStoryboard, data: PlacePageData) {
    self.interactor = interactor
    self.storyboard = storyboard
    self.placePageData = data
  }

  private func configureViewControllers() -> [UIViewController] {
    var viewControllers = [UIViewController]()
    viewControllers.append(elevationMapViewController)

    return viewControllers
  }

  func calculateSteps(inScrollView scrollView: UIScrollView, compact: Bool) -> [PlacePageState] {
    var steps: [PlacePageState] = []
    let scrollHeight = scrollView.height
    let previewHeight = elevationMapViewController.getPreviewHeight()
    steps.append(.closed(-scrollHeight))
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
