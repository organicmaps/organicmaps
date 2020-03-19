class PlacePageElevationLayout: IPlacePageLayout {
  private var placePageData: PlacePageData
  private var interactor: PlacePageInteractor
  private let storyboard: UIStoryboard
  weak var presenter: PlacePagePresenterProtocol?

  lazy var viewControllers: [UIViewController] = {
    return configureViewControllers()
  }()

  var actionBar: ActionBarViewController? = nil
  var adState: AdBannerState = .unset

  lazy var previewViewController: PlacePagePreviewViewController = {
    let vc = storyboard.instantiateViewController(ofType: PlacePagePreviewViewController.self)
    vc.placePagePreviewData = placePageData.previewData
    vc.delegate = interactor
    return vc
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
//    viewControllers.append(previewViewController)
    viewControllers.append(elevationMapViewController)

    return viewControllers
  }

  func calculateSteps(inScrollView scrollView: UIScrollView) -> [PlacePageState] {
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
    return steps
  }
}
