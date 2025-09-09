@objcMembers
final class NavigationDashboardViewController: UIViewController {

  private enum Constants {
    static let navigationInfoViewXibName = "MWMNavigationInfoView"
    static let navigationControlViewXibName = "NavigationControlView"

    static let grabberHeight: CGFloat = 5
    static let grabberWidth: CGFloat = 36
    static let grabberTopInset: CGFloat = 5

    static let closeButtonInsets = UIEdgeInsets(top: 0, left: 0, bottom: 0, right: -16)
    static let closeButtonSize: CGSize = CGSize(width: 28, height: 28)

    static let settingsButtonSize: CGFloat = 32
    static let settingsButtonInsetRight: CGFloat = -16
    static let settingsButtonSpacing: CGFloat = 8

    static let transportOptionsCollectionInsets = UIEdgeInsets(top: 6, left: 16, bottom: 0, right: -36)
    static let transportOptionsCollectionHeight: CGFloat = 44

    static let routePointsInsets = UIEdgeInsets(top: 8, left: 16, bottom: -8, right: -16)

    static let routeStatusInsets = UIEdgeInsets(top: 8, left: 16, bottom: 0, right: -16)
    static let routeStatusStackSpacing: CGFloat = 4
    static let startButtonSpacing: CGFloat = 4

    static let routePointsDiscoverabilityPadding: CGFloat = 20
    static let panGestureThreshold: CGFloat = 5
  }

  typealias StepsController = ModalPresentationStepsController<NavigationDashboardModalPresentationStep>

  // MARK: - UI Components
  @objc let availableAreaView = SearchOnMapAreaView()
  private let grabberView = UIView()
  private let closeButton = CircleImageButton()
  private var transportOptionsView = TransportOptionsView()
  private let estimatesStackView = UIStackView()
  private let routeStatusStackView = UIStackView()
  private let estimatesView = EstimatesView()
  private let transportTransitStepsView = TransportTransitStepsView()
  private let elevationProfileView = ElevationProfileView()
  private let settingsButton = UIButton(type: .system)
  private let settingsBadge = BadgeWithNumber()
  private var routePointsView = RoutePointsView()
  private let bottomActionsMenu = RouteActionsBottomMenuView()
  private let searchButton = UIButton()
  private let bookmarksButton = UIButton()
  private let saveRouteAsTrackButton = UIButton()
  private let startRouteButton = StartRouteButton()
  private var navigationInfoView: NavigationInfoView!
  private var navigationControlView: NavigationControlView!
  private let impactGenerator = UIImpactFeedbackGenerator(style: .medium)
  private var internalScrollViewContentOffset: CGFloat = .zero

  var interactor: NavigationDashboard.Interactor?
  private var presentationStepStrategy = NavigationDashboardModalPresentationStepStrategy()
  private var presentationStepsController: StepsController!

  // MARK: - Init
  init() {
    super.init(nibName: nil, bundle: nil)
    self.configureModalPresentation()
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  // MARK: - Lifecycle
  override func loadView() {
    view = TouchTransparentView()
    guard let navigationInfoView = Bundle.main.loadNibNamed(Constants.navigationInfoViewXibName,
                                                            owner: nil,
                                                            options: nil)?.first as? NavigationInfoView,
          let navigationControlView = Bundle.main.loadNibNamed(Constants.navigationControlViewXibName,
                                                               owner: nil,
                                                               options: nil)?.first as? NavigationControlView else {
      fatalError("Failed to load NavigationInfoView or NavigationControlView from nib")
    }
    navigationInfoView.ownerView = view
    navigationControlView.ownerView = view
    navigationControlView.delegate = interactor?.delegate
    self.navigationInfoView = navigationInfoView
    self.navigationControlView = navigationControlView
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    setupView()
    layout()
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    let routePointsBottomPoint = availableAreaView.convert(routePointsView.contentBottom, to: view)
    bottomActionsMenu.setShadowVisible(routePointsBottomPoint.y + Constants.startButtonSpacing > bottomActionsMenu.origin.y)
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    updateFrameOfPresentedViewInContainerView()
    transportOptionsView.reload()
  }

  override func viewWillTransition(to size: CGSize, with coordinator: any UIViewControllerTransitionCoordinator) {
    super.viewWillTransition(to: size, with: coordinator)
    routePointsView.viewWillTransition(to: size, with: coordinator)
    if #available(iOS 14.0, *), ProcessInfo.processInfo.isiOSAppOnMac {
      updateFrameOfPresentedViewInContainerView()
    }
  }

  func add(to parentViewController: MapViewController) {
    parentViewController.addChild(self)
    parentViewController.view.insertSubview(view, belowSubview: parentViewController.placePageContainer)
    view.frame = parentViewController.view.bounds
    view.autoresizingMask = [.flexibleWidth, .flexibleHeight]
    view.setNeedsLayout()
    view.layoutIfNeeded()
    didMove(toParent: parentViewController)
  }

  // MARK: - Setup Views
  private func setupView() {
    view.backgroundColor = .clear
    setupAvailableView()
    setupGrabberView()
    setupCloseButton()
    setupEstimatesView()
    setupRouteStatusView()
    setupSettingsButton()
    setupBottomMenuActions()
    setupTransportOptionsView()
    setupRoutePointsView()
    setupGestureRecognizers()
    impactGenerator.prepare()
  }

  private func configureModalPresentation() {
    let stepsController = StepsController(presentedView: availableAreaView,
                                          containerViewController: self,
                                          stepStrategy: presentationStepStrategy,
                                          currentStep: .hidden,
                                          didUpdateHandler: presentationUpdateHandler)
    presentationStepsController = stepsController
  }

  private var presentationUpdateHandler: (StepsController.StepUpdate) -> Void {
    { [weak self] update in
      guard let self else { return }
      switch update {
      case .didClose:
        self.interactor?.process(.close)
      case .didUpdateFrame(let frame):
        self.interactor?.process(.updateVisibleAreaInsets(visibleAreaInsets(for: frame)))
      case .didUpdateStep(let step):
        self.interactor?.process(.didUpdatePresentationStep(step))
        break
      }
    }
  }

  private func visibleAreaInsets(for frame: CGRect) -> UIEdgeInsets {
    let isCompact = traitCollection.verticalSizeClass == .compact
    let bottom = (isCompact || isiPad) ? 0 : frame.height - frame.origin.y
    let left = isiPad ? frame.origin.x + frame.width : 0
    return UIEdgeInsets(top: 0, left: left, bottom: bottom, right: 0)
  }

  private func setupGestureRecognizers() {
    iPhoneSpecific {
      let panGestureRecognizer = UIPanGestureRecognizer(target: self, action: #selector(handlePan(_:)))
      panGestureRecognizer.delegate = self
      availableAreaView.addGestureRecognizer(panGestureRecognizer)
    }
  }

  @objc
  private func handlePan(_ gesture: UIPanGestureRecognizer) {
    presentationStepsController.handlePan(gesture)
  }

  private func setupAvailableView() {
    availableAreaView.setStyle(.modalSheetBackground)
  }

  private func setupGrabberView() {
    // TODO: remove when the grabber will be the same on all the modal screens
    grabberView.layer.setCornerRadius(.grabber)
    grabberView.backgroundColor = .blackDividers()
    iPadSpecific { [weak self] in
      self?.grabberView.isHidden = true
    }
  }

  private func setupCloseButton() {
    closeButton.setImage(UIImage(resource: .icClose))
    closeButton.addTarget(self, action: #selector(didTapCloseButton), for: .touchUpInside)
    closeButton.setStyle(.black)
  }

  private func setupEstimatesView() {
    estimatesStackView.axis = .horizontal
    estimatesStackView.distribution = .equalSpacing
  }

  private func setupRouteStatusView() {
    routeStatusStackView.axis = .vertical
    routeStatusStackView.distribution = .equalSpacing
    routeStatusStackView.spacing = Constants.routeStatusStackSpacing
  }

  private func setupSettingsButton() {
    settingsButton.setStyle(.blue)
    settingsButton.setImage(UIImage(resource: .icMenuSettings), for: .normal)
    settingsButton.addTarget(self, action: #selector(didTapSettingsButton), for: .touchUpInside)
    settingsBadge.badgeAddTo(settingsButton)
    settingsBadge.isHidden = true
  }

  private func setupTransportOptionsView() {
    transportOptionsView.interactor = interactor
  }

  private func setupBottomMenuActions() {
    searchButton.setStyle(.flatNormalGrayButtonBig)
    searchButton.setImage(UIImage(resource: .icMenuSearch), for: .normal)
    searchButton.addTarget(self, action: #selector(didTapSearchButton), for: .touchUpInside)

    bookmarksButton.setStyle(.flatNormalGrayButtonBig)
    bookmarksButton.setImage(UIImage(resource: .icMenuBookmarkList), for: .normal)
    bookmarksButton.addTarget(self, action: #selector(didTapBookmarksButton), for: .touchUpInside)

    saveRouteAsTrackButton.setStyle(.flatNormalGrayButtonBig)
    saveRouteAsTrackButton.setImage(UIImage(resource: .ic24PxImport), for: .normal)
    saveRouteAsTrackButton.addTarget(self, action: #selector(didTapSaveRouteAsTrackButton), for: .touchUpInside)

    startRouteButton.addTarget(self, action: #selector(didTapStartRouteButton), for: .touchUpInside)

    bottomActionsMenu.addActionView(searchButton)
    bottomActionsMenu.addActionView(bookmarksButton)
    bottomActionsMenu.addActionView(saveRouteAsTrackButton)
    bottomActionsMenu.addActionView(startRouteButton)
  }

  private func setupRoutePointsView() {
    routePointsView.interactor = interactor
    routePointsView.scrollViewDelegate = self
  }

  // MARK: - Actions

  @objc
  private func didTapCloseButton() {
    interactor?.process(.close)
  }

  @objc
  private func didTapSettingsButton() {
    interactor?.process(.settingsButtonDidTap)
  }

  @objc
  private func didTapSearchButton() {
    impactGenerator.impactOccurred()
    interactor?.process(.searchButtonDidTap)
  }

  @objc
  private func didTapBookmarksButton() {
    impactGenerator.impactOccurred()
    interactor?.process(.bookmarksButtonDidTap)
  }

  @objc
  private func didTapSaveRouteAsTrackButton() {
    impactGenerator.impactOccurred()
    saveRouteAsTrackButton.isEnabled = false
    interactor?.process(.saveRouteAsTrackButtonDidTap)
  }

  @objc
  private func didTapStartRouteButton() {
    impactGenerator.impactOccurred()
    interactor?.process(.startButtonDidTap)
  }

  // MARK: - Layout
  private func layout() {
    view.addSubview(availableAreaView)
    availableAreaView.addSubview(grabberView)
    availableAreaView.addSubview(closeButton)
    availableAreaView.addSubview(transportOptionsView)

    estimatesStackView.addArrangedSubview(estimatesView)
    estimatesStackView.addArrangedSubview(settingsButton)

    routeStatusStackView.addArrangedSubview(estimatesStackView)
    routeStatusStackView.addArrangedSubview(transportTransitStepsView)
    routeStatusStackView.addArrangedSubview(elevationProfileView)
    availableAreaView.addSubview(routeStatusStackView)

    availableAreaView.addSubview(routePointsView)
    view.addSubview(bottomActionsMenu)

    grabberView.translatesAutoresizingMaskIntoConstraints = false
    closeButton.translatesAutoresizingMaskIntoConstraints = false
    transportOptionsView.translatesAutoresizingMaskIntoConstraints = false
    routeStatusStackView.translatesAutoresizingMaskIntoConstraints = false
    settingsButton.translatesAutoresizingMaskIntoConstraints = false
    routePointsView.translatesAutoresizingMaskIntoConstraints = false
    bottomActionsMenu.translatesAutoresizingMaskIntoConstraints = false

    routeStatusStackView.setContentHuggingPriority(.defaultHigh, for: .vertical)
    closeButton.setContentHuggingPriority(.defaultHigh, for: .horizontal)
    settingsButton.setContentHuggingPriority(.defaultHigh, for: .horizontal)

    NSLayoutConstraint.activate([
      grabberView.centerXAnchor.constraint(equalTo: availableAreaView.centerXAnchor),
      grabberView.widthAnchor.constraint(equalToConstant: Constants.grabberWidth),
      grabberView.topAnchor.constraint(equalTo: availableAreaView.topAnchor, constant: Constants.grabberTopInset),
      grabberView.heightAnchor.constraint(equalToConstant: Constants.grabberHeight),

      closeButton.trailingAnchor.constraint(equalTo: availableAreaView.safeAreaLayoutGuide.trailingAnchor, constant: Constants.closeButtonInsets.right),
      closeButton.topAnchor.constraint(equalTo: transportOptionsView.topAnchor, constant: Constants.closeButtonInsets.top),
      closeButton.widthAnchor.constraint(equalToConstant: Constants.closeButtonSize.width),
      closeButton.heightAnchor.constraint(equalToConstant: Constants.closeButtonSize.height),

      transportOptionsView.leadingAnchor.constraint(equalTo: availableAreaView.safeAreaLayoutGuide.leadingAnchor, constant: Constants.transportOptionsCollectionInsets.left),
      transportOptionsView.trailingAnchor.constraint(equalTo: closeButton.leadingAnchor, constant: Constants.transportOptionsCollectionInsets.right),
      transportOptionsView.topAnchor.constraint(equalTo: grabberView.bottomAnchor, constant: Constants.transportOptionsCollectionInsets.top),
      transportOptionsView.heightAnchor.constraint(equalToConstant: Constants.transportOptionsCollectionHeight),

      routeStatusStackView.leadingAnchor.constraint(equalTo: availableAreaView.leadingAnchor, constant: Constants.routeStatusInsets.left),
      routeStatusStackView.trailingAnchor.constraint(equalTo: availableAreaView.trailingAnchor, constant: Constants.routeStatusInsets.right),
      routeStatusStackView.topAnchor.constraint(equalTo: transportOptionsView.bottomAnchor, constant: Constants.routeStatusInsets.top),

      settingsButton.heightAnchor.constraint(equalToConstant: Constants.settingsButtonSize),
      settingsButton.widthAnchor.constraint(equalTo: settingsButton.heightAnchor),

      routePointsView.leadingAnchor.constraint(equalTo: availableAreaView.leadingAnchor, constant: Constants.routePointsInsets.left),
      routePointsView.trailingAnchor.constraint(equalTo: availableAreaView.trailingAnchor, constant: Constants.routePointsInsets.right),
      routePointsView.topAnchor.constraint(equalTo: routeStatusStackView.bottomAnchor, constant: Constants.routePointsInsets.top),
      routePointsView.bottomAnchor.constraint(equalTo: availableAreaView.bottomAnchor, constant: Constants.routePointsInsets.bottom),

      bottomActionsMenu.leadingAnchor.constraint(equalTo: availableAreaView.leadingAnchor),
      bottomActionsMenu.trailingAnchor.constraint(equalTo: availableAreaView.trailingAnchor),
      bottomActionsMenu.bottomAnchor.constraint(equalTo: view.bottomAnchor),
    ])
    presentationStepsController.setInitialState()
  }

  private func updateFrameOfPresentedViewInContainerView() {
    presentationStepsController.updateFrame()
    view.layoutIfNeeded()
  }

  private func updatePresentationStep(_ step: NavigationDashboardModalPresentationStep) {
    availableAreaView.layoutIfNeeded()
    let regularHeight = routePointsView.contentBottom.y
    + bottomActionsMenu.frame.height
    + Constants.startButtonSpacing

    let compactHeight = routePointsView.origin.y
    + bottomActionsMenu.frame.height
    + Constants.startButtonSpacing
    + Constants.routePointsDiscoverabilityPadding

    let shouldForceContentFrameUpdate = presentationStepStrategy.regularHeigh != regularHeight || presentationStepStrategy.compactHeight != compactHeight
    presentationStepStrategy.regularHeigh = regularHeight
    presentationStepStrategy.compactHeight = compactHeight
    presentationStepsController.setStep(step, forced: shouldForceContentFrameUpdate)
  }

  private func close() {
    navigationControlView.isVisible = false
    bottomActionsMenu.setHidden(true)
    willMove(toParent: nil)
    presentationStepsController.close { [weak self] in
      self?.view.removeFromSuperview()
      self?.removeFromParent()
    }
  }
}

// MARK: - UIGestureRecognizerDelegate
extension NavigationDashboardViewController: UIGestureRecognizerDelegate {
  func gestureRecognizerShouldBegin(_ gestureRecognizer: UIGestureRecognizer) -> Bool {
    true
  }

  func gestureRecognizer(_ gestureRecognizer: UIGestureRecognizer, shouldRecognizeSimultaneouslyWith otherGestureRecognizer: UIGestureRecognizer) -> Bool {
    if gestureRecognizer is UIPanGestureRecognizer && otherGestureRecognizer is UIPanGestureRecognizer {
      // threshold is used to soften transition from the internal scroll zero content offset
      return internalScrollViewContentOffset < Constants.panGestureThreshold
    }
    return false
  }
}

// MARK: - SearchOnMapScrollViewDelegate
extension NavigationDashboardViewController: UIScrollViewDelegate {
  func scrollViewDidScroll(_ scrollView: UIScrollView) {
    let hasContainerReachedTheTop = Int(availableAreaView.frame.origin.y) <= Int(presentationStepsController.maxAvailableFrame.origin.y)
    let isScrollViewScrollingDown = scrollView.contentOffset.y <= 0
    let scrollViewHasZeroContentOffset = abs(internalScrollViewContentOffset) < Constants.panGestureThreshold
    let shouldLockScrollView =
      (scrollViewHasZeroContentOffset && hasContainerReachedTheTop && isScrollViewScrollingDown) ||
      (!hasContainerReachedTheTop && scrollViewHasZeroContentOffset)
    if shouldLockScrollView {
      scrollView.contentOffset.y = 0
    }
    internalScrollViewContentOffset = scrollView.contentOffset.y
  }
}

// MARK: - Public Methods

extension NavigationDashboardViewController {
  func render(_ viewModel: NavigationDashboard.ViewModel) {
    switch viewModel.dashboardState {
    case .closed:
      close()
      return

    case .navigation:
      navigationInfoView.onNavigationInfoUpdated(viewModel.entity)
      navigationInfoView.updateToastView()
      if let navigationSearchState = viewModel.navigationSearchState {
        navigationInfoView.setSearchState(navigationSearchState, animated: true)
      }
      navigationControlView.isVisible = true
      navigationControlView.onNavigationInfoUpdated(viewModel.entity)

    case .error:
      estimatesView.setState(viewModel.estimatesState)
      transportTransitStepsView.setNavigationInfo(nil)
      elevationProfileView.setImage(nil)
      saveRouteAsTrackButton.isEnabled = viewModel.canSaveRouteAsTrack
      navigationControlView.isVisible = false

    case .prepare, .ready, .planning:
      transportOptionsView.set(transportOptions: viewModel.transportOptions,
                               selectedRouterType: viewModel.routerType)
      estimatesView.setState(viewModel.estimatesState)
      transportTransitStepsView.setNavigationInfo(viewModel.entity)
      elevationProfileView.setImage(viewModel.elevationInfo?.image)
      routePointsView.setRoutePoints(viewModel.routePoints)
      settingsBadge.isHidden = !viewModel.routingOptions.hasOptions
      settingsBadge.number = viewModel.routingOptions.enabledOptionsCount
      saveRouteAsTrackButton.isEnabled = viewModel.canSaveRouteAsTrack
      navigationControlView.isVisible = false

    case .hidden:
      break

    @unknown default:
      fatalError("Unknown dashboard state: \(viewModel.dashboardState)")
    }

    navigationInfoView.state = viewModel.navigationInfo.state
    navigationInfoView.availableArea = viewModel.navigationInfo.availableArea

    bottomActionsMenu.setHidden(viewModel.isBottomActionsMenuHidden)
    startRouteButton.setState(viewModel.startButtonState)

    updatePresentationStep(viewModel.presentationStep)
  }
}
