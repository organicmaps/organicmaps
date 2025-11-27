protocol PlacePageViewProtocol: AnyObject {
  var interactor: PlacePageInteractorProtocol? { get set }
  var view: UIView! { get }

  func showNextStop()
  func layoutIfNeeded()
  func updateWithLayout(_ layout: IPlacePageLayout)
  func showAlert(_ alert: UIAlertController)
}

final class PlacePageScrollView: UIScrollView {
  override func point(inside point: CGPoint, with event: UIEvent?) -> Bool {
    return point.y > 0
  }

  override func scrollRectToVisible(_ rect: CGRect, animated: Bool) {
    guard isScrollEnabled else { return }
    super.scrollRectToVisible(rect, animated: animated)
  }
}

@objc final class PlacePageViewController: UIViewController {
  
  private enum Constants {
    static let actionBarHeight: CGFloat = 50
    static let additionalPreviewOffset: CGFloat = 80
    static let fastSwipeDownVelocity: CGFloat = -3.0
    static let fastSwipeUpVelocity: CGFloat = 2.0
  }
  
  @IBOutlet private var scrollView: PlacePageScrollView!
  @IBOutlet private var stackView: UIStackView!
  @IBOutlet private var actionBarContainerView: UIView!
  @IBOutlet private var actionBarHeightConstraint: NSLayoutConstraint!
  @IBOutlet private var panGesture: UIPanGestureRecognizer!

  private let headerStackView = UIStackView()
  private let backgroundView = UIView()
  private var beginDragging = false
  private var previousTraitCollection: UITraitCollection?
  private var scrollSteps: [PlacePageState] = []
  private var previousScrollContentOffset: CGPoint?
  private var userDefinedStep: PlacePageState?
  private var isNavigationBarVisible = false
  private var isFirstOpening = true
  private var isVisible: Bool = false

  var layout: IPlacePageLayout!
  var interactor: PlacePageInteractorProtocol?
  var isPreviewPlus: Bool = false

  // MARK: - VC Lifecycle

  override func viewDidLoad() {
    super.viewDidLoad()

    MWMKeyboard.add(self)

    setupView()
    setupLayout(layout)
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    guard !layout.headerViewController.isEditingTitle else { return }
    if #available(iOS 13.0, *) {
      // See https://github.com/organicmaps/organicmaps/issues/6917 for the details.
    } else if previousTraitCollection == nil {
      scrollView.contentInset = alternativeSizeClass(iPhone: UIEdgeInsets(top: scrollView.height, left: 0, bottom: 0, right: 0),
                                                     iPad: UIEdgeInsets.zero)
      updateSteps()
    }
    panGesture.isEnabled = alternativeSizeClass(iPhone: false, iPad: true)
    previousTraitCollection = traitCollection
    updateBackgroundViewFrame()
  }

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)
    isVisible = true
    updatePreviewOffset()
  }

  override func viewWillDisappear(_ animated: Bool) {
    super.viewWillDisappear(animated)
    previousScrollContentOffset = scrollView.contentOffset
    isVisible = false
    interactor?.viewWillDisappear()
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    // Update layout when the device was rotated but skip when the appearance was changed.
    if self.previousTraitCollection != nil, previousTraitCollection?.userInterfaceStyle == traitCollection.userInterfaceStyle, previousTraitCollection?.verticalSizeClass != traitCollection.verticalSizeClass {
      // Skip updating steps if the title is being edited because the header is pinned to the keyboard.
      guard !self.layout.headerViewController.isEditingTitle else { return }
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
    updateBackgroundViewFrame()
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
    view.insertSubview(backgroundView, at: 0)
    backgroundView.setStyle(.ppBackgroundView)

    scrollView.decelerationRate = .fast
    scrollView.setStyle(.ppView)

    headerStackView.axis = .vertical
    headerStackView.distribution = .fill

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
    let showSeparator = layout.sectionSpacing > 0
    stackView.spacing = layout.sectionSpacing
    fillHeader(with: layout.headerViewControllers, showSeparator: showSeparator)
    fillBody(with: layout.bodyViewControllers, showSeparator: showSeparator)

    layout.headerViewController.onEditingTitle = { [weak self] isEditing in
      self?.scrollView.isScrollEnabled = !isEditing
    }

    scrollView.isScrollEnabled = true
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
      if isFirstOpening {
        updateTopBound(bound)
        isFirstOpening = false
      }
      ModalPresentationAnimator.animate(animations: { [weak scrollView] in
        scrollView?.contentOffset = contentOffset
        self.updateBackgroundViewFrame()
        self.layoutIfNeeded()
      }, completion: { complete in
        if complete {
          completion?()
        }
      })
    } else {
      scrollView?.contentOffset = contentOffset
      self.updateBackgroundViewFrame()
      completion?()
    }
  }

  private func showLastStop() {
    if let lastStop = scrollSteps.last {
      scrollTo(CGPoint(x: 0, y: lastStop.offset), forced: true)
      userDefinedStep = lastStop
    }
  }

  private func updateTopBound(_ bound: CGFloat, updatingViewport: Bool = true) {
    alternativeSizeClass(iPhone: {
      let isCompact = traitCollection.verticalSizeClass == .compact
      let insets = UIEdgeInsets(top: 0, left: 0, bottom: isCompact ? 0 : bound, right: 0)
      self.interactor?.updateVisibleAreaInsets(insets, updatingViewport: updatingViewport)
    }, iPad: {})
  }

  private func updateBackgroundViewFrame() {
    backgroundView.frame = CGRect(x: 0,
                                  y: max(scrollView.origin.y, -scrollView.contentOffset.y),
                                  width: scrollView.width,
                                  height: stackView.height + actionBarContainerView.height)
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
    cleanupLayout()
    self.layout = layout
    setupLayout(layout)
    updatePreviewOffset()
  }
  
  private func hideActionBar(_ value: Bool) {
    actionBarHeightConstraint.constant = !value ? Constants.actionBarHeight : .zero
  }

  func updatePreviewOffset() {
    updateSteps()
    guard !beginDragging, isVisible else { return }
    let estimatedYOffset = isPreviewPlus ? scrollSteps[2].offset : scrollSteps[1].offset + Constants.additionalPreviewOffset
    var offset = CGPoint(x: 0, y: estimatedYOffset) 
    if let userDefinedStep {
      // Respect user defined offset if possible.
      switch userDefinedStep {
      case .preview:
        offset.y = scrollSteps[1].offset
      case .previewPlus(let yOffset):
        offset.y = yOffset
      case .full:
        offset.y = previousScrollContentOffset?.y ?? scrollSteps.last?.offset ?? 0
      case .closed:
        break
      }
    } else if let previousScrollContentOffset {
      // Keep previous offset during layout update if possible.
      offset.y = max(estimatedYOffset, previousScrollContentOffset.y)
    }
    scrollTo(offset)
  }

  func showNextStop() {
    guard scrollView.isScrollEnabled else { return }
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
          let frame = self.view.frame.offsetBy(dx: 0, dy: self.view.height - self.stackView.convert(self.stackView.origin, to: self.view).y)
          self.view.frame = frame
        }, iPad: {
          self.view.alpha = 0
        })
      },
      completion: { _ in
        completion()
        MWMKeyboard.remove(self)
      })
  }

  func showAlert(_ alert: UIAlertController) {
    present(alert, animated: true)
  }
}

// MARK: - UIScrollViewDelegate

extension PlacePageViewController: UIScrollViewDelegate {
  func scrollViewDidScroll(_ scrollView: UIScrollView) {
    previousScrollContentOffset = scrollView.contentOffset

    if scrollView.contentOffset.y < -scrollView.height + 1 && beginDragging {
      interactor?.close()
    }
    let bound = view.height + scrollView.contentOffset.y
    updateTopBound(bound, updatingViewport: false) // Skip updating viewport on every drag.
    onOffsetChanged(scrollView.contentOffset.y)
    updateBackgroundViewFrame()
  }

  func scrollViewWillBeginDragging(_ scrollView: UIScrollView) {
    beginDragging = true
  }

  func scrollViewWillEndDragging(_ scrollView: UIScrollView,
                                 withVelocity velocity: CGPoint,
                                 targetContentOffset: UnsafeMutablePointer<CGPoint>) {
    if velocity.y < Constants.fastSwipeDownVelocity {
      interactor?.close()
      return
    }
    if velocity.y > Constants.fastSwipeUpVelocity {
      showLastStop()
      return
    }
    
    let maxOffset = scrollSteps.last?.offset ?? 0
    if targetContentOffset.pointee.y > maxOffset {
      return
    }

    let nextStep = findNextStop(scrollView.contentOffset.y, velocity: velocity.y)
    targetContentOffset.pointee = CGPoint(x: 0, y: nextStep.offset)
    userDefinedStep = nextStep
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

  private func contentOffsetForTitleEditing() -> CGPoint {
    let visibleScrollHeight = scrollView.frame.height + actionBarContainerView.frame.height - MWMKeyboard.keyboardHeight()
    let yOffsetFromKeyboard = layout.headerViewController.view.height
    return CGPoint(x: scrollView.contentOffset.x, y: yOffsetFromKeyboard - visibleScrollHeight)
  }
}

// MARK: - MWMKeyboardObserver

extension PlacePageViewController: MWMKeyboardObserver {
  func onKeyboardWillAnimate() {
    guard !isiPad, isVisible else { return }
    scrollView.contentOffset = contentOffsetForTitleEditing()
  }

  func onKeyboardAnimation() {}
}
