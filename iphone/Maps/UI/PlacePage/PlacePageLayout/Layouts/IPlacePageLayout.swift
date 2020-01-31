enum PlacePageState {
  case closed(CGFloat)
  case preview(CGFloat)
  case previewPlus(CGFloat)
  case expanded(CGFloat)

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
    }
  }
}

protocol IPlacePageLayout: class {
  var presenter: PlacePagePresenterProtocol? { get set }
  var viewControllers: [UIViewController] { get }
  var actionBar: UIViewController? { get }
  var adState: AdBannerState { get set }

  func calculateSteps(inScrollView scrollView: UIScrollView) -> [PlacePageState]
}
