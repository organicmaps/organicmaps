protocol PlacePageViewProtocol: class {
  var presenter: PlacePagePresenterProtocol!  { get set }
  var scrollView: UIScrollView! { get set }

  func addToStack(_ viewController: UIViewController)
  func addActionBar(_ actionBarViewController: UIViewController)
  func hideActionBar(_ value: Bool)
  func scrollTo(_ point: CGPoint)
  func layoutIfNeeded()
}

final class PlacePageScrollView: UIScrollView {
  override func point(inside point: CGPoint, with event: UIEvent?) -> Bool {
    return point.y > 0
  }
}

final class TouchTransparentView: UIView {
  var targetView: UIView?
  override func point(inside point: CGPoint, with event: UIEvent?) -> Bool {
    guard let targetView = targetView else {
      return super.point(inside: point, with: event)
    }
    let targetPoint = convert(point, to: targetView)
    return targetView.point(inside: targetPoint, with: event)
  }
}

@objc final class PlacePageViewController: UIViewController {
  @IBOutlet var scrollView: UIScrollView!
  @IBOutlet var stackView: UIStackView!
  @IBOutlet var actionBarContainerView: UIView!
  @IBOutlet var actionBarHeightConstraint: NSLayoutConstraint!

  var presenter: PlacePagePresenterProtocol!

  var beginDragging = false
  var rootViewController: MapViewController {
    MapViewController.shared()
  }

  let kActionBarHeight:CGFloat = 50

  // MARK: - VC Lifecycle

  override func viewDidLoad() {
    super.viewDidLoad()
    presenter?.configure()

    if let touchTransparentView = view as? TouchTransparentView {
      touchTransparentView.targetView = scrollView
    }

    let bgView = UIView()
    bgView.styleName = "Background"
    stackView.insertSubview(bgView, at: 0)
    bgView.alignToSuperview()
    scrollView.decelerationRate = .fast
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    if traitCollection.horizontalSizeClass == .compact {
      scrollView.contentInset = UIEdgeInsets(top: scrollView.height, left: 0, bottom: 0, right: 0)
    }
  }

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)
    if !beginDragging {
      presenter?.updatePreviewOffset()
    }
  }
}

extension PlacePageViewController: PlacePageViewProtocol {
  func hideActionBar(_ value: Bool) {
    actionBarHeightConstraint.constant = !value ? kActionBarHeight : 0
  }

  func addToStack(_ viewController: UIViewController) {
    addChild(viewController)
    stackView.addArrangedSubview(viewController.view)
    viewController.didMove(toParent: self)
  }

  func addActionBar(_ actionBarViewController: UIViewController) {
    actionBarViewController.view.translatesAutoresizingMaskIntoConstraints = false
    actionBarContainerView.addSubview(actionBarViewController.view)
    NSLayoutConstraint.activate([
      actionBarViewController.view.leadingAnchor.constraint(equalTo: actionBarContainerView.leadingAnchor),
      actionBarViewController.view.topAnchor.constraint(equalTo: actionBarContainerView.topAnchor),
      actionBarViewController.view.trailingAnchor.constraint(equalTo: actionBarContainerView.trailingAnchor),
      actionBarViewController.view.bottomAnchor.constraint(equalTo: view.safeAreaLayoutGuide.bottomAnchor)
    ])
  }

  func scrollTo(_ point: CGPoint) {
    UIView.animate(withDuration: kDefaultAnimationDuration) { [weak scrollView] in
      scrollView?.contentOffset = point
    }
  }

  func layoutIfNeeded() {
    view.layoutIfNeeded()
  }
}

// MARK: - UIScrollViewDelegate

extension PlacePageViewController: UIScrollViewDelegate {
  func scrollViewDidScroll(_ scrollView: UIScrollView) {
    if scrollView.contentOffset.y < -scrollView.height + 1 && beginDragging {
      rootViewController.dismissPlacePage()
    }
  }

  func scrollViewWillBeginDragging(_ scrollView: UIScrollView) {
    beginDragging = true
  }

  func scrollViewWillEndDragging(_ scrollView: UIScrollView,
                                 withVelocity velocity: CGPoint,
                                 targetContentOffset: UnsafeMutablePointer<CGPoint>) {
    let maxOffset = presenter.maxOffset
    if targetContentOffset.pointee.y > maxOffset {
      presenter?.setAdState(.detailed)
      return
    }

    let targetState = presenter.findNextStop(scrollView.contentOffset.y, velocity: velocity.y)
    if targetState.offset > scrollView.contentSize.height - scrollView.contentInset.top {
      presenter?.setAdState(.detailed)
      return
    }
    switch targetState {
    case .closed(_):
      fallthrough
    case .preview(_):
      presenter?.setAdState(.compact)
    case .previewPlus(_):
      fallthrough
    case .expanded(_):
      presenter?.setAdState(.detailed)
    }
    targetContentOffset.pointee = CGPoint(x: 0, y: targetState.offset)
  }
}
