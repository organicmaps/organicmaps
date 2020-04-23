enum PlacePageState {
  case closed(CGFloat)
  case preview(CGFloat)
  case previewPlus(CGFloat)
  case expanded(CGFloat)
  case full(CGFloat)

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
    case .full(let value):
      return value
    }
  }
}

protocol IPlacePageLayout: class {
  var presenter: PlacePagePresenterProtocol? { get set }
  var header: PlacePageHeaderViewController? { get }
  var viewControllers: [UIViewController] { get }
  var actionBar: ActionBarViewController? { get }
  var navigationBar: UIViewController? { get }
  var adState: AdBannerState { get set }

  func calculateSteps(inScrollView scrollView: UIScrollView, compact: Bool) -> [PlacePageState]
}
