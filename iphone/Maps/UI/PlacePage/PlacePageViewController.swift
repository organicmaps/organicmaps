protocol PlacePageViewProtocol: class {
  var presenter: PlacePagePresenterProtocol!  { get set }
  var scrollView: UIScrollView! { get set }
  var beginDragging: Bool { get set }

  func addToStack(_ viewController: UIViewController)
  func addActionBar(_ actionBarViewController: UIViewController)
  func hideActionBar(_ value: Bool)
  func addNavigationBar(_ header: UIViewController)
  func scrollTo(_ point: CGPoint, animated: Bool, forced: Bool, completion: (()->())?)
  func layoutIfNeeded()
}

extension PlacePageViewProtocol {
  func scrollTo(_ point: CGPoint, animated: Bool = true, forced: Bool = false, completion: (()->())? = nil) {
    scrollTo(point, animated: animated, forced: forced, completion: completion)
  }
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
    bgView.styleName = "PPBackgroundView"
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
    presenter?.updatePreviewOffset()
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

  func addNavigationBar(_ header: UIViewController) {
    header.view.translatesAutoresizingMaskIntoConstraints = false
    view.addSubview(header.view)
    addChild(header)
    NSLayoutConstraint.activate([
      header.view.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      header.view.topAnchor.constraint(equalTo: view.topAnchor),
      header.view.trailingAnchor.constraint(equalTo: view.trailingAnchor),
    ])
  }

  func scrollTo(_ point: CGPoint, animated: Bool, forced: Bool, completion: (()->())?) {
    if traitCollection.horizontalSizeClass != .compact || (beginDragging && !forced) {
      return
    }
    let scrollPosition = CGPoint(x: point.x, y: min((scrollView.contentSize.height - scrollView.height), point.y))
    if animated {
      UIView.animate(withDuration: kDefaultAnimationDuration, animations: { [weak scrollView] in
        scrollView?.contentOffset = scrollPosition
      }) { (complete) in
        if complete {
          completion?()
        }
      }
    } else {
      scrollView?.contentOffset = scrollPosition
      completion?()
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
    presenter.onOffsetChanged(scrollView.contentOffset.y)
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
      fallthrough
    case .full(_):
      presenter?.setAdState(.detailed)
    }
    targetContentOffset.pointee = CGPoint(x: 0, y: targetState.offset)
  }
}
