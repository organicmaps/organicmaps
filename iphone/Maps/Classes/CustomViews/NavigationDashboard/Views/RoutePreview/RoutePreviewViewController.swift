@objcMembers
final class RoutePreviewViewController: UIViewController {

  private enum Constants {
    static let grabberHeight: CGFloat = 5
    static let grabberWidth: CGFloat = 36
    static let grabberTopInset: CGFloat = 5

    static let backButtonInsets = UIEdgeInsets(top: 4, left: 16, bottom: 0, right: 16)
    static let backButtonSize: CGSize = CGSize(width: 24, height: 40)

    static let settingsButtonSize: CGFloat = 32
    static let settingsButtonInsetRight: CGFloat = -16
    static let settingsButtonSpacing: CGFloat = 8

    static let transportOptionsCollectionInsets = UIEdgeInsets(top: 6, left: 16, bottom: 0, right: -16)
    static let transportOptionsCollectionHeight: CGFloat = 44

    static let routePointsInsets = UIEdgeInsets(top: 8, left: 0, bottom: -8, right: 0)

    static let routeStatusInsets = UIEdgeInsets(top: 8, left: 16, bottom: 0, right: -16)
    static let routeStatusStackSpacing: CGFloat = 4
  }

  // MARK: - UI Components
  private let grabberView = UIView()
  private let backButton = UIButton(type: .system)
  private var transportOptionsView = TransportOptionsView()
  private let estimatesStackView = UIStackView()
  private let routeStatusStackView = UIStackView()
  private let estimatesLabel = UILabel()
  private let transportTransitStepsView = TransportTransitStepsView()
  private let elevationProfileView = ElevationProfileView()
  private let settingsButton = UIButton(type: .system)
  private var routePointsView = RoutePointsView()
  private let startButton = StartRouteButton()
  private let availableAreaView = SearchOnMapAreaView()

  var interactor: RoutePreview.Interactor?
  private var viewModel: RoutePreview.ViewModel = .initial
  private let presentationStepsController = ModalPresentationStepsController()

  // MARK: - Init
  init() {
    super.init(nibName: nil, bundle: nil)
    configureModalPresentation()
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  // MARK: - Lifecycle
  override func loadView() {
    view = TouchTransparentView()
  }

  // MARK: - Lifecycle
  override func viewDidLoad() {
    super.viewDidLoad()
    setupView()
    layout()
  }

  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    view.layoutIfNeeded()
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    updateFrameOfPresentedViewInContainerView()
    transportOptionsView.reload()
  }

  override func viewWillTransition(to size: CGSize, with coordinator: any UIViewControllerTransitionCoordinator) {
    super.viewWillTransition(to: size, with: coordinator)
    if #available(iOS 14.0, *), ProcessInfo.processInfo.isiOSAppOnMac {
      updateFrameOfPresentedViewInContainerView()
    }
  }

  func add(to parentViewController: MapViewController) {
    parentViewController.addChild(self)
    parentViewController.view.addSubview(view)
    view.frame = parentViewController.view.bounds
    view.autoresizingMask = [.flexibleWidth, .flexibleHeight]
    view.setNeedsLayout()
    view.layoutIfNeeded()
    didMove(toParent: parentViewController)
  }

  // MARK: - Setup Views
  private func setupView() {
    view.backgroundColor = .clear
    availableAreaView.setStyle(.modalSheetBackground)
    setupGrabberView()
    setupBackButton()
    setupEstimatesView()
    setupRouteStatusView()
    setupSettingsButton()
    setupStartButton()
    setupTransportOptionsView()
    setupRoutePointsView()
    configureModalPresentation()
    setupGestureRecognizers()
  }

  private func configureModalPresentation() {
    guard let mapViewController = MapViewController.shared() else {
      fatalError("MapViewController is not available")
    }
    presentationStepsController.set(presentedView: availableAreaView, containerViewController: self)
    presentationStepsController.didUpdateHandler = presentationUpdateHandler

    // TODO: affect side buttons area
//    let affectedAreaViews = [
//      mapViewController.sideButtonsArea,
//      mapViewController.trafficButtonArea,
//    ]
//    affectedAreaViews.forEach { $0?.addAffectingView(availableAreaView) }
  }

  private var presentationUpdateHandler: (ModalPresentationStepsController.StepUpdate) -> Void {
    { [weak self] update in
      guard let self else { return }
      switch update {
      case .didClose:
        self.interactor?.process(.close)
      case .didUpdateFrame(let frame):
        self.interactor?.process(.updatePresentationFrame(frame))
      case .didUpdateStep(let step):
//        self.interactor?.process(.didUpdatePresentationStep(step))
        break
      }
    }
  }

  private func setupGestureRecognizers() {
    iPhoneSpecific {
      let panGestureRecognizer = UIPanGestureRecognizer(target: self, action: #selector(handlePan(_:)))
      availableAreaView.addGestureRecognizer(panGestureRecognizer)
    }
  }

  @objc
  private func handlePan(_ gesture: UIPanGestureRecognizer) {
    presentationStepsController.handlePan(gesture)
  }

  private func setupGrabberView() {
    // TODO: remove when the grabber will be the same on all the modal screens
    grabberView.layer.setCornerRadius(.grabber)
    grabberView.backgroundColor = .blackDividers()
    iPadSpecific { [weak self] in
      self?.grabberView.isHidden = true
    }
  }

  private func setupBackButton() {
    backButton.setImage(UIImage(resource: .icNavBarBack), for: .normal)
    backButton.matchInterfaceOrientation()
    backButton.addTarget(self, action: #selector(didTapBackButton), for: .touchUpInside)
    backButton.setStyle(.black)
  }

  @objc
  private func didTapBackButton() {
    interactor?.process(.goBack)
  }

  private func setupEstimatesView() {
    estimatesLabel.setFontStyleAndApply(.bold14)
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
  }

  private func setupTransportOptionsView() {
    transportOptionsView.interactor = interactor
  }

  private func setupStartButton() {
    startButton.setOnTapAction { [weak self] in
      self?.interactor?.process(.startNavigation)
    }
  }

  private func setupRoutePointsView() {
    routePointsView.interactor = interactor
  }

  // MARK: - Layout Constraints
  private func layout() {
    view.addSubview(availableAreaView)
    availableAreaView.addSubview(grabberView)
    availableAreaView.addSubview(backButton)
    availableAreaView.addSubview(transportOptionsView)

    estimatesStackView.addArrangedSubview(estimatesLabel)
    estimatesStackView.addArrangedSubview(settingsButton)

    routeStatusStackView.addArrangedSubview(estimatesStackView)
    routeStatusStackView.addArrangedSubview(transportTransitStepsView)
    routeStatusStackView.addArrangedSubview(elevationProfileView)
    availableAreaView.addSubview(routeStatusStackView)

    availableAreaView.addSubview(routePointsView)
    view.addSubview(startButton)

    grabberView.translatesAutoresizingMaskIntoConstraints = false
    backButton.translatesAutoresizingMaskIntoConstraints = false
    transportOptionsView.translatesAutoresizingMaskIntoConstraints = false
    routeStatusStackView.translatesAutoresizingMaskIntoConstraints = false
    settingsButton.translatesAutoresizingMaskIntoConstraints = false
    routePointsView.translatesAutoresizingMaskIntoConstraints = false
    startButton.translatesAutoresizingMaskIntoConstraints = false

    routeStatusStackView.setContentHuggingPriority(.defaultHigh, for: .vertical)
    backButton.setContentHuggingPriority(.defaultHigh, for: .horizontal)
    settingsButton.setContentHuggingPriority(.defaultHigh, for: .horizontal)

    NSLayoutConstraint.activate([
      grabberView.centerXAnchor.constraint(equalTo: availableAreaView.centerXAnchor),
      grabberView.widthAnchor.constraint(equalToConstant: Constants.grabberWidth),
      grabberView.topAnchor.constraint(equalTo: availableAreaView.topAnchor, constant: Constants.grabberTopInset),
      grabberView.heightAnchor.constraint(equalToConstant: Constants.grabberHeight),

      backButton.leadingAnchor.constraint(equalTo: availableAreaView.safeAreaLayoutGuide.leadingAnchor, constant: Constants.backButtonInsets.left),
      backButton.widthAnchor.constraint(equalToConstant: Constants.backButtonSize.width),
      backButton.heightAnchor.constraint(equalToConstant: Constants.backButtonSize.height),
      backButton.centerYAnchor.constraint(equalTo: transportOptionsView.centerYAnchor),

      transportOptionsView.leadingAnchor.constraint(equalTo: backButton.trailingAnchor, constant: Constants.transportOptionsCollectionInsets.left),
      transportOptionsView.trailingAnchor.constraint(equalTo: availableAreaView.trailingAnchor, constant: Constants.transportOptionsCollectionInsets.right),
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

      startButton.leadingAnchor.constraint(equalTo: availableAreaView.leadingAnchor),
      startButton.trailingAnchor.constraint(equalTo: availableAreaView.trailingAnchor),
      startButton.bottomAnchor.constraint(equalTo: view.bottomAnchor)
    ])
    presentationStepsController.setInitialState()
  }

  private func updateFrameOfPresentedViewInContainerView() {
    presentationStepsController.updateMaxAvailableFrame()
    availableAreaView.frame = presentationStepsController.currentFrame
    view.layoutIfNeeded()
  }
}

// MARK: - Public Methods

extension RoutePreviewViewController {
  func render(_ newViewModel: RoutePreview.ViewModel) {
    guard !newViewModel.shouldClose else {
      close()
      return
    }

    let shouldReloadRoutePoints = viewModel.routePoints != newViewModel.routePoints
    viewModel = newViewModel
    if shouldReloadRoutePoints {
      routePointsView.setRoutePoints(newViewModel.routePoints)
    }
    transportOptionsView.set(transportOptions: newViewModel.transportOptions,
                             selectedRouterType: newViewModel.routerType)
    startButton.setState(newViewModel.startButtonState)
    presentationStepsController.setStep(newViewModel.presentationStep)
    estimatesLabel.attributedText = newViewModel.estimates
    transportTransitStepsView.setNavigationInfo(newViewModel.entity)
    elevationProfileView.setImage(viewModel.elevationInfo?.image)
    // TODO: update the navigation view
  }

  func close() {
    startButton.setState(.hidden)
    willMove(toParent: nil)
    presentationStepsController.close { [weak self] in
      self?.view.removeFromSuperview()
      self?.removeFromParent()
    }
  }
}
