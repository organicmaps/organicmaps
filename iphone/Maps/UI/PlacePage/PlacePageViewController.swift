protocol PlacePageViewProtocol: class {
  var presenter: PlacePagePresenterProtocol! { get set }
  var scrollView: UIScrollView! { get set }
  var beginDragging: Bool { get set }
  var traitCollection: UITraitCollection { get }

  func addHeader(_ headerViewController: UIViewController)
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
  private var previousTraitCollection: UITraitCollection?

  let kActionBarHeight:CGFloat = 50

  // MARK: - VC Lifecycle

  override func viewDidLoad() {
    super.viewDidLoad()
    presenter?.configure()

    let bgView = UIView()
    bgView.styleName = "PPBackgroundView"
    stackView.insertSubview(bgView, at: 0)
    bgView.alignToSuperview()
    scrollView.decelerationRate = .fast
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    if previousTraitCollection == nil {
      scrollView.contentInset = alternative(iPhone: UIEdgeInsets(top: scrollView.height, left: 0, bottom: 0, right: 0),
                                            iPad: UIEdgeInsets.zero)
      presenter.updateSteps()
    }
    self.previousTraitCollection = self.traitCollection
  }

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)
    presenter?.updatePreviewOffset()
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    if self.previousTraitCollection != nil {
      DispatchQueue.main.async {
        self.presenter.updateSteps()
        self.presenter.showLastStop()
        self.scrollView.contentInset = alternative(iPhone: UIEdgeInsets(top: self.scrollView.height, left: 0, bottom: 0, right: 0),
                                                   iPad: UIEdgeInsets.zero)
      }
    }
  }
}

extension PlacePageViewController: PlacePageViewProtocol {
  override var traitCollection: UITraitCollection {
    get {
      return super.traitCollection
    }
  }

  func hideActionBar(_ value: Bool) {
    actionBarHeightConstraint.constant = !value ? kActionBarHeight : 0
  }

  func addHeader(_ headerViewController: UIViewController) {
    addToStack(headerViewController)
    // TODO: workaround. Custom spacing isn't working if visibility of any arranged subview
    // changes after setting custom spacing
    DispatchQueue.main.async {
      self.stackView.setCustomSpacing(0, after: headerViewController.view)
    }
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
    if alternative(iPhone: beginDragging, iPad: true) && !forced {
      return
    }
    if forced {
      beginDragging = true
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
