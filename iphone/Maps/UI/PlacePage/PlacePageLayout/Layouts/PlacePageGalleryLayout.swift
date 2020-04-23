final class PlacePageGalleryLayout: IPlacePageLayout {
  weak var presenter: PlacePagePresenterProtocol?
  var header: PlacePageHeaderViewController? { nil }
  var actionBar: ActionBarViewController? { nil }
  var navigationBar: UIViewController? { nil }
  var adState: AdBannerState = .unset

  var viewControllers: [UIViewController] {
    [guidesGalleryViewController]
  }

  lazy var guidesGalleryViewController: GuidesGalleryViewController = {
    GuidesGalleryBuilder.build()
  }()

  func calculateSteps(inScrollView scrollView: UIScrollView, compact: Bool) -> [PlacePageState] {
    var steps: [PlacePageState] = []
    let scrollHeight = scrollView.height
    steps.append(.closed(-scrollHeight))
    guard let previewView = guidesGalleryViewController.view else {
      return steps
    }
    let previewFrame = scrollView.convert(previewView.bounds, from: previewView)
    steps.append(.preview(previewFrame.maxY - scrollHeight))
    return steps
  }
}
