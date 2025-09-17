protocol PlacePageViewProtocol: AnyObject {
  var interactor: PlacePageInteractorProtocol? { get set }
  var view: UIView! { get }

  func setLayout(_ layout: IPlacePageLayout)
  func updatePreviewOffset(reset: Bool)
  func showNextStop()
  func layoutIfNeeded()
  func updateWithLayout(_ layout: IPlacePageLayout)
  func showAlert(_ alert: UIAlertController)
}

final class PlacePageScrollView: UIScrollView {
  override func point(inside point: CGPoint, with event: UIEvent?) -> Bool {
    return point.y > 0
  }
}

@objc final class PlacePageViewController: UIViewController {
  
  private enum Constants {
    static let actionBarHeight: CGFloat = 50
    static let additionalPreviewOffset: CGFloat = 80
  }
  
  @IBOutlet private var scrollView: UIScrollView!
  @IBOutlet private var stackView: UIStackView!
  @IBOutlet private var actionBarContainerView: UIView!
  @IBOutlet private var actionBarDivider: UIView!
  @IBOutlet private var actionBarHeightConstraint: NSLayoutConstraint!
  @IBOutlet private var panGesture: UIPanGestureRecognizer!

  private let headerStackView = UIStackView()
  private let backgroundView = UIView()
  private var beginDragging = false
  private var previousTraitCollection: UITraitCollection?
  private var layout: IPlacePageLayout!
  private var scrollSteps: [PlacePageState] = []
  private var previousScrollContentOffset: CGPoint?
  private var isNavigationBarVisible = false

  var interactor: PlacePageInteractorProtocol?
  var isPreviewPlus: Bool = false

  // MARK: - VC Lifecycle

  override func viewDidLoad() {
    super.viewDidLoad()

    setupView()
    setupLayout(layout)
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    if #available(iOS 13.0, *) {
      // See https://github.com/organicmaps/organicmaps/issues/6917 for the details.
    } else if previousTraitCollection == nil {
      scrollView.contentInset = alternativeSizeClass(iPhone: UIEdgeInsets(top: scrollView.height, left: 0, bottom: 0, right: 0),
                                                     iPad: UIEdgeInsets.zero)
      updateSteps()
    }
    panGesture.isEnabled = alternativeSizeClass(iPhone: false, iPad: true)
    previousTraitCollection = traitCollection
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    interactor?.viewWillAppear()
  }

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)
    updatePreviewOffset(reset: false)
  }

  override func viewWillDisappear(_ animated: Bool) {
    super.viewWillDisappear(animated)
    interactor?.viewWillDisappear()
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    // Update layout when the device was rotated but skip when the appearance was changed.
    if self.previousTraitCollection != nil, previousTraitCollection?.userInterfaceStyle == traitCollection.userInterfaceStyle, previousTraitCollection?.verticalSizeClass != traitCollection.verticalSizeClass {
      DispatchQueue.main.async {
        self.updateSteps()
        self.showLastStop()
        self.scrollView.contentInset = self.alternativeSizeClass(iPhone: UIEdgeInsets(top: self.scrollView.height, left: 0, bottom: 0, right: 0),
                                                                 iPad: UIEdgeInsets.zero)
      }
    }
  }

  // MARK: - Actions

  @IBAction func onPan(gesture: UIPanGestureRecognizer) {
    let xOffset = gesture.translation(in: view.superview).x
    gesture.setTranslation(CGPoint.zero, in: view.superview)
    view.minX += xOffset
    view.minX = min(view.minX, 0)
    let alpha = view.maxX / view.width
    view.alpha = alpha

    let state = gesture.state
    if state == .ended || state == .cancelled {
      if alpha < 0.8 {
        interactor?.close()
      } else {
        UIView.animate(withDuration: kDefaultAnimationDuration) {
          self.view.minX = 0
          self.view.alpha = 1
        }
      }
    }
  }

  // MARK: - Private methods

  private func updateSteps() {
    layoutIfNeeded()
    scrollSteps = layout.calculateSteps(inScrollView: scrollView,
                                        compact: traitCollection.verticalSizeClass == .compact)
  }

  private func findNextStop(_ offset: CGFloat, velocity: CGFloat) -> PlacePageState {
    if velocity == 0 {
      return findNearestStop(offset)
    }

    var result: PlacePageState
    if velocity < 0 {
      guard let first = scrollSteps.first else { return .closed(-scrollView.height) }
      result = first
      scrollSteps.suffix(from: 1).forEach {
        if offset > $0.offset {
          result = $0
        }
      }
    } else {
      guard let last = scrollSteps.last else { return .closed(-scrollView.height) }
      result = last
      scrollSteps.reversed().suffix(from: 1).forEach {
        if offset < $0.offset {
          result = $0
        }
      }
    }

    return result
  }

  private func setupView() {
    stackView.insertSubview(backgroundView, at: 0)
    backgroundView.alignToSuperview()

    headerStackView.axis = .vertical
    headerStackView.distribution = .fill

    scrollView.decelerationRate = .fast
    scrollView.backgroundColor = .clear

    let topCorners: CACornerMask = [.layerMinXMinYCorner, .layerMaxXMinYCorner]
    stackView.layer.setCornerRadius(.modalSheet, maskedCorners: topCorners)
    stackView.backgroundColor = .clear

    if isiPad {
      let bottomCorners: CACornerMask = [.layerMinXMaxYCorner, .layerMaxXMaxYCorner]
      actionBarContainerView.layer.setCornerRadius(.modalSheet, maskedCorners: bottomCorners)
      actionBarContainerView.layer.masksToBounds = true
    }

    // See https://github.com/organicmaps/organicmaps/issues/6917 for the details.
    if #available(iOS 13.0, *), previousTraitCollection == nil {
      scrollView.contentInset = alternativeSizeClass(iPhone: UIEdgeInsets(top: view.height, left: 0, bottom: 0, right: 0),
                                                     iPad: UIEdgeInsets.zero)
      scrollView.layoutIfNeeded()
    }
  }

  private func setupLayout(_ layout: IPlacePageLayout) {
    setLayout(layout)

    let showSeparator = layout.sectionSpacing > 0
    stackView.spacing = layout.sectionSpacing
    fillHeader(with: layout.headerViewControllers, showSeparator: showSeparator)
    fillBody(with: layout.bodyViewControllers, showSeparator: showSeparator)

    beginDragging = false
    if let actionBar = layout.actionBar {
      hideActionBar(false)
      addActionBar(actionBar)
    } else {
      hideActionBar(true)
    }
  }

  private func fillHeader(with viewControllers: [UIViewController], showSeparator: Bool = true) {
    viewControllers.forEach { [self] viewController in
      if !stackView.arrangedSubviews.contains(headerStackView) {
        stackView.addArrangedSubview(headerStackView)
      }
      headerStackView.addArrangedSubview(viewController.view)
    }
    if showSeparator {
      headerStackView.addSeparator(.bottom)
    }
  }

  private func fillBody(with viewControllers: [UIViewController], showSeparator: Bool = true) {
    viewControllers.forEach { [self] viewController in
      addChild(viewController)
      stackView.addArrangedSubview(viewController.view)
      viewController.didMove(toParent: self)
      if showSeparator {
        viewController.view.addSeparator(.top)
        viewController.view.addSeparator(.bottom)
      }
    }
  }

  private func cleanupLayout() {
    guard let layout else { return }
    let childViewControllers = [layout.actionBar, layout.navigationBar] + layout.headerViewControllers + layout.bodyViewControllers
    childViewControllers.forEach {
      $0?.willMove(toParent: nil)
      $0?.view.removeFromSuperview()
      $0?.removeFromParent()
    }
    headerStackView.arrangedSubviews.forEach { $0.removeFromSuperview() }
    stackView.arrangedSubviews.forEach { $0.removeFromSuperview() }
  }

  private func findNearestStop(_ offset: CGFloat) -> PlacePageState {
    var result = scrollSteps[0]
    scrollSteps.suffix(from: 1).forEach { ppState in
      if abs(result.offset - offset) > abs(ppState.offset - offset) {
        result = ppState
      }
    }
    return result
  }

  private func addActionBar(_ actionBarViewController: UIViewController) {
    addChild(actionBarViewController)
    actionBarViewController.view.translatesAutoresizingMaskIntoConstraints = false
    actionBarContainerView.addSubview(actionBarViewController.view)
    actionBarViewController.didMove(toParent: self)
    NSLayoutConstraint.activate([
      actionBarViewController.view.leadingAnchor.constraint(equalTo: actionBarContainerView.leadingAnchor),
      actionBarViewController.view.topAnchor.constraint(equalTo: actionBarContainerView.topAnchor),
      actionBarViewController.view.trailingAnchor.constraint(equalTo: actionBarContainerView.trailingAnchor),
      actionBarViewController.view.bottomAnchor.constraint(equalTo: view.safeAreaLayoutGuide.bottomAnchor)
    ])
  }

  private func addNavigationBar(_ header: UIViewController) {
    header.view.translatesAutoresizingMaskIntoConstraints = false
    view.addSubview(header.view)
    addChild(header)
    NSLayoutConstraint.activate([
      header.view.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      header.view.topAnchor.constraint(equalTo: view.topAnchor),
      header.view.trailingAnchor.constraint(equalTo: view.trailingAnchor)
    ])
  }

  private func scrollTo(_ point: CGPoint, animated: Bool = true, forced: Bool = false, completion: (() -> Void)? = nil) {
    if alternativeSizeClass(iPhone: beginDragging, iPad: true) && !forced {
      return
    }
    if forced {
      beginDragging = true
    }
    let contentOffset = CGPoint(x: point.x, y: min(scrollView.contentSize.height - scrollView.height, point.y))
    let bound = view.frame.height + contentOffset.y
    if animated {
      updateTopBound(bound)
      UIView.animate(withDuration: kDefaultAnimationDuration, animations: { [weak scrollView] in
        scrollView?.contentOffset = contentOffset
        self.layoutIfNeeded()
      }) { complete in
        if complete {
          completion?()
        }
      }
    } else {
      scrollView?.contentOffset = contentOffset
      completion?()
    }
  }

  private func showLastStop() {
    if let lastStop = scrollSteps.last {
      scrollTo(CGPoint(x: 0, y: lastStop.offset), forced: true)
    }
  }

  private func updateTopBound(_ bound: CGFloat) {
    alternativeSizeClass(iPhone: {
      let isCompact = traitCollection.verticalSizeClass == .compact
      let insets = UIEdgeInsets(top: 0, left: 0, bottom: isCompact ? 0 : bound, right: 0)
      self.interactor?.updateVisibleAreaInsets(insets)
    }, iPad: {})
  }
}

// MARK: - PlacePageViewProtocol

extension PlacePageViewController: PlacePageViewProtocol {
  func layoutIfNeeded() {
    guard layout != nil else { return }
    view.layoutIfNeeded()
  }

  func updateWithLayout(_ layout: IPlacePageLayout) {
    previousScrollContentOffset = scrollView.contentOffset
    setupLayout(layout)
  }
  
  func setLayout(_ layout: IPlacePageLayout) {
    if self.layout != nil {
      cleanupLayout()
    }
    self.layout = layout
  }

  private func hideActionBar(_ value: Bool) {
    actionBarHeightConstraint.constant = !value ? Constants.actionBarHeight : .zero
  }

  func updatePreviewOffset(reset: Bool = true) {
    updateSteps()
    guard !beginDragging else { return }
    let offset: CGPoint
    // Keep previous offset during layout update if possible.
    if !reset,
       let previousScrollContentOffset,
       let maxYOffset = scrollSteps.last?.offset {
      offset = CGPoint(x: 0, y: min(previousScrollContentOffset.y, maxYOffset))
    } else {
      offset = CGPoint(x: 0, y: isPreviewPlus ? scrollSteps[2].offset : scrollSteps[1].offset + Constants.additionalPreviewOffset)
    }
    scrollTo(offset)
  }

  func showNextStop() {
    if let nextStop = scrollSteps.last(where: { $0.offset > scrollView.contentOffset.y }) {
      scrollTo(CGPoint(x: 0, y: nextStop.offset), forced: true)
    }
  }

  @objc
  func close(completion: @escaping (() -> Void)) {
    view.isUserInteractionEnabled = false
    updateTopBound(.zero)
    ModalPresentationAnimator.animate(
      animations: {
        self.alternativeSizeClass(iPhone: {
          let frame = self.view.frame.offsetBy(dx: 0, dy: self.stackView.height + self.actionBarContainerView.frame.height)
          self.view.frame = frame
        }, iPad: {
          let frame = self.view.frame
          self.view.minX = frame.minX - frame.width
          self.view.alpha = 0
        })
      },
      completion: { _ in
        completion()
      })
  }

  func showAlert(_ alert: UIAlertController) {
    present(alert, animated: true)
  }
}

// MARK: - UIScrollViewDelegate

extension PlacePageViewController: UIScrollViewDelegate {
  func scrollViewDidScroll(_ scrollView: UIScrollView) {
    if scrollView.contentOffset.y < -scrollView.height + 1 && beginDragging {
      interactor?.close()
    }
    onOffsetChanged(scrollView.contentOffset.y)
    let bound = view.height + scrollView.contentOffset.y
    updateTopBound(bound)
  }

  func scrollViewWillBeginDragging(_ scrollView: UIScrollView) {
    beginDragging = true
  }

  func scrollViewWillEndDragging(_ scrollView: UIScrollView,
                                 withVelocity velocity: CGPoint,
                                 targetContentOffset: UnsafeMutablePointer<CGPoint>) {
    print("velocity", velocity)
    let maxOffset = scrollSteps.last?.offset ?? 0
    if targetContentOffset.pointee.y > maxOffset {
      return
    }

    let targetState = findNextStop(scrollView.contentOffset.y, velocity: velocity.y)
    if targetState.offset > scrollView.contentSize.height - scrollView.contentInset.top {
      return
    }

    updateSteps()
    let nextStep = findNextStop(scrollView.contentOffset.y, velocity: velocity.y)
    targetContentOffset.pointee = CGPoint(x: 0, y: nextStep.offset)
  }

  private func onOffsetChanged(_ offset: CGFloat) {
    if offset > 0 && !isNavigationBarVisible {
      setNavigationBarVisible(true)
    } else if offset <= 0 && isNavigationBarVisible {
      setNavigationBarVisible(false)
    }
  }

  private func setNavigationBarVisible(_ visible: Bool) {
    guard visible != isNavigationBarVisible, let navigationBar = layout?.navigationBar else { return }
    isNavigationBarVisible = visible
    if isNavigationBarVisible {
      addNavigationBar(navigationBar)
    } else {
      navigationBar.removeFromParent()
      navigationBar.view.removeFromSuperview()
    }
  }
}
