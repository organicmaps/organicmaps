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
    static let transportOptionsItemSize = CGSize(width: 40, height: 40)

    static let routePointsInsets = UIEdgeInsets(top: 8, left: 0, bottom: -8, right: 0)
    static let routePointsVerticalSpacing: CGFloat = 8
    static let routePointCellHeight: CGFloat = 52
    static let addRoutePointCellHeight: CGFloat = 30

    static let etaLabelTopSpacing: CGFloat = 8
    static let etaLabelLeadingPadding: CGFloat = 16
  }

  // MARK: - UI Components
  private let grabberView = UIView()
  private let backButton = UIButton(type: .system)
  private var transportOptionsCollectionView: UICollectionView!
  private let estimatesPreview = RouteEstimatesPreviewView()
  private let settingsButton = UIButton(type: .system)
  private var routePointsCollectionView: UICollectionView!
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
    transportOptionsCollectionView.reloadData()
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
    setupSettingsButton()
    setupStartButton()
    setupTransportOptionsCollection()
    setupRoutePointsCollectionView()
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

  private func setupSettingsButton() {
    settingsButton.setStyle(.blue)
    settingsButton.setImage(UIImage(resource: .icMenuSettings), for: .normal)
  }

  private func setupTransportOptionsCollection() {
    let layout = UICollectionViewFlowLayout()
    layout.scrollDirection = .horizontal
    transportOptionsCollectionView = UICollectionView(frame: .zero, collectionViewLayout: layout)
    transportOptionsCollectionView.backgroundColor = .clear
    transportOptionsCollectionView.dataSource = self
    transportOptionsCollectionView.delegate = self
    transportOptionsCollectionView.showsHorizontalScrollIndicator = false
    transportOptionsCollectionView.showsVerticalScrollIndicator = false
    transportOptionsCollectionView.isScrollEnabled = false
    transportOptionsCollectionView.allowsMultipleSelection = false
    transportOptionsCollectionView.register(cell: TransportOptionCollectionViewCell.self)
  }

  private func setupStartButton() {
    startButton.setOnTapAction { [weak self] in
      self?.interactor?.process(.startNavigation)
    }
  }

  private func setupRoutePointsCollectionView() {
    let layout = UICollectionViewFlowLayout()
    layout.scrollDirection = .vertical
    routePointsCollectionView = UICollectionView(frame: .zero, collectionViewLayout: layout)
    routePointsCollectionView.backgroundColor = .clear
    routePointsCollectionView.dragInteractionEnabled = true
    routePointsCollectionView.alwaysBounceVertical = false
    routePointsCollectionView.isScrollEnabled = false
    routePointsCollectionView.dragDelegate = self
    routePointsCollectionView.dropDelegate = self
    routePointsCollectionView.dataSource = self
    routePointsCollectionView.delegate = self
    routePointsCollectionView.register(cell: RouteStopCollectionViewCell.self)
    routePointsCollectionView.register(cell: AddItemCollectionViewCell.self)
  }

  // MARK: - Layout Constraints
  private func layout() {
    view.addSubview(availableAreaView)
    availableAreaView.addSubview(grabberView)
    availableAreaView.addSubview(backButton)
    availableAreaView.addSubview(transportOptionsCollectionView)
    availableAreaView.addSubview(estimatesPreview)
    availableAreaView.addSubview(settingsButton)
    availableAreaView.addSubview(routePointsCollectionView)
    view.addSubview(startButton)

    grabberView.translatesAutoresizingMaskIntoConstraints = false
    backButton.translatesAutoresizingMaskIntoConstraints = false
    transportOptionsCollectionView.translatesAutoresizingMaskIntoConstraints = false
    estimatesPreview.translatesAutoresizingMaskIntoConstraints = false
    settingsButton.translatesAutoresizingMaskIntoConstraints = false
    routePointsCollectionView.translatesAutoresizingMaskIntoConstraints = false
    startButton.translatesAutoresizingMaskIntoConstraints = false

    estimatesPreview.setContentHuggingPriority(.defaultHigh, for: .vertical)
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
      backButton.centerYAnchor.constraint(equalTo: transportOptionsCollectionView.centerYAnchor),

      transportOptionsCollectionView.leadingAnchor.constraint(equalTo: backButton.trailingAnchor, constant: Constants.transportOptionsCollectionInsets.left),
      transportOptionsCollectionView.trailingAnchor.constraint(equalTo: availableAreaView.trailingAnchor, constant: Constants.transportOptionsCollectionInsets.right),
      transportOptionsCollectionView.topAnchor.constraint(equalTo: grabberView.bottomAnchor, constant: Constants.transportOptionsCollectionInsets.top),
      transportOptionsCollectionView.heightAnchor.constraint(equalToConstant: Constants.transportOptionsCollectionHeight),

      estimatesPreview.leadingAnchor.constraint(equalTo: availableAreaView.leadingAnchor, constant: Constants.etaLabelLeadingPadding),
      estimatesPreview.topAnchor.constraint(equalTo: transportOptionsCollectionView.bottomAnchor, constant: Constants.etaLabelTopSpacing),

      settingsButton.leadingAnchor.constraint(equalTo: estimatesPreview.trailingAnchor, constant: Constants.settingsButtonSpacing),
      settingsButton.trailingAnchor.constraint(equalTo: availableAreaView.trailingAnchor, constant: Constants.settingsButtonInsetRight),
      settingsButton.centerYAnchor.constraint(equalTo: estimatesPreview.centerYAnchor),
      settingsButton.heightAnchor.constraint(equalToConstant: Constants.settingsButtonSize),
      settingsButton.widthAnchor.constraint(equalTo: settingsButton.heightAnchor),

      routePointsCollectionView.leadingAnchor.constraint(equalTo: availableAreaView.leadingAnchor, constant: Constants.routePointsInsets.left),
      routePointsCollectionView.trailingAnchor.constraint(equalTo: availableAreaView.trailingAnchor, constant: Constants.routePointsInsets.right),
      routePointsCollectionView.topAnchor.constraint(equalTo: estimatesPreview.bottomAnchor, constant: Constants.routePointsInsets.top),
      routePointsCollectionView.bottomAnchor.constraint(equalTo: availableAreaView.bottomAnchor, constant: Constants.routePointsInsets.bottom),

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

// MARK: - UICollectionViewDataSource, UICollectionViewDelegate
extension RoutePreviewViewController: UICollectionViewDataSource, UICollectionViewDelegate {
  func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
    switch collectionView {
    case transportOptionsCollectionView:
      return viewModel.transportOptions.count
    case routePointsCollectionView:
      return viewModel.routePoints.count + 1
    default:
      fatalError("Unknown collection view")
    }
  }

  func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
    switch collectionView {
    case transportOptionsCollectionView:
      let cell = collectionView.dequeueReusableCell(cell: TransportOptionCollectionViewCell.self, indexPath: indexPath)
      let routerType = viewModel.transportOptions[indexPath.item]
      cell.configure(with: routerType)
      if viewModel.routerType == routerType {
        collectionView.selectItem(at: indexPath, animated: true, scrollPosition: [])
      }
      return cell

    case routePointsCollectionView:
      switch indexPath.item {
      case viewModel.routePoints.count:
        let cell = collectionView.dequeueReusableCell(cell: AddItemCollectionViewCell.self, indexPath: indexPath)
        cell.didTapAction = { [weak self] in
          self?.interactor?.process(.addRoutePoint)
        }
        return cell
      default:
        let cell = collectionView.dequeueReusableCell(cell: RouteStopCollectionViewCell.self, indexPath: indexPath)
        let routePoints = viewModel.routePoints
        cell.configure(with: routePoints.cellViewModel(for: indexPath.item, onCloseHandler: { [weak self] in
          if let point = routePoints[indexPath.item] {
            self?.interactor?.process(.deleteRoutePoint(point))
          }
        }))
        return cell
      }
    default:
      fatalError("Unknown collection view")
    }
  }

  func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    switch collectionView {
    case transportOptionsCollectionView:
      let routerType = viewModel.transportOptions[indexPath.item]
      interactor?.process(.selectRouterType(routerType))

    case routePointsCollectionView:
      switch indexPath.item {
      case viewModel.routePoints.count:
        break // Add point cell
      default:
        // TODO: point may by unavailable
        let point = viewModel.routePoints[indexPath.item]
        interactor?.process(.selectRoutePoint(point, at: indexPath.item))
        break
      }

    default:
      fatalError("Unknown collection view")
    }
  }
}

// MARK: - UICollectionViewDelegateFlowLayout
extension RoutePreviewViewController: UICollectionViewDelegateFlowLayout {
  func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, sizeForItemAt indexPath: IndexPath) -> CGSize {
    switch collectionView {
    case transportOptionsCollectionView:
      return Constants.transportOptionsItemSize

    case routePointsCollectionView:
      let isAddPointCell = indexPath.item == viewModel.routePoints.count
      let height = isAddPointCell ? Constants.addRoutePointCellHeight : Constants.routePointCellHeight
      return CGSize(width: collectionView.bounds.width,
                    height: height)

    default:
      fatalError("Unknown collection view")
    }
  }

  func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, minimumLineSpacingForSectionAt section: Int) -> CGFloat {
    switch collectionView {
    case transportOptionsCollectionView:
      // fit to the width
      let cellsCount = CGFloat(viewModel.transportOptions.count)
      let size = (collectionView.width - Constants.transportOptionsItemSize.width * cellsCount) / (cellsCount - 1)
      return size

    case routePointsCollectionView:
      return Constants.routePointsVerticalSpacing

    default:
      fatalError("Unknown collection view")
    }
  }
}

// MARK: - UICollectionViewDragDelegate, UICollectionViewDropDelegate
extension RoutePreviewViewController: UICollectionViewDragDelegate, UICollectionViewDropDelegate {
  func collectionView(_ collectionView: UICollectionView, itemsForBeginning session: UIDragSession, at indexPath: IndexPath) -> [UIDragItem] {
    guard indexPath.item < viewModel.routePoints.count else { return [] }
    let item = viewModel.routePoints[indexPath.item]
    let title = viewModel.routePoints.title(for: indexPath.item)
    let itemProvider = NSItemProvider(object: title as NSString)
    let dragItem = UIDragItem(itemProvider: itemProvider)
    dragItem.localObject = item
    return [dragItem]
  }

  func collectionView(_ collectionView: UICollectionView, performDropWith coordinator: UICollectionViewDropCoordinator) {
    guard let destinationIndexPath = coordinator.destinationIndexPath,
          destinationIndexPath.item < viewModel.routePoints.count else { return }

    for item in coordinator.items {
      if let sourceIndexPath = item.sourceIndexPath, let _ = item.dragItem.localObject as? MWMRoutePoint {
        guard sourceIndexPath != destinationIndexPath else { return }
        interactor?.process(.moveRoutePoint(from: sourceIndexPath.item, to: destinationIndexPath.item))
        coordinator.drop(item.dragItem, toItemAt: destinationIndexPath)
        routePointsCollectionView.reloadItems(at: [sourceIndexPath, destinationIndexPath])
      }
    }
  }

  func collectionView(_ collectionView: UICollectionView, canHandle session: UIDropSession) -> Bool {
    true
  }

  func collectionView(_ collectionView: UICollectionView, dropSessionDidUpdate session: UIDropSession, withDestinationIndexPath destinationIndexPath: IndexPath?) -> UICollectionViewDropProposal {
    guard let destinationIndexPath = destinationIndexPath,
          destinationIndexPath.item < viewModel.routePoints.count else {
      return UICollectionViewDropProposal(operation: .forbidden)
    }
    return UICollectionViewDropProposal(operation: .move, intent: .insertAtDestinationIndexPath)
  }
}

// MARK: - Public Methods

extension RoutePreviewViewController {
  func render(_ newViewModel: RoutePreview.ViewModel) {
    print(#function)
    guard !newViewModel.shouldClose else {
      close()
      return
    }

    let shouldReloadTransportOptions = viewModel.routerType != newViewModel.routerType
    let shouldReloadRoutePoints = viewModel.routePoints != newViewModel.routePoints
    viewModel = newViewModel
    if shouldReloadRoutePoints {
      routePointsCollectionView.reloadData()
    }
    if shouldReloadTransportOptions {
      transportOptionsCollectionView.reloadData()
    }
    startButton.setState(newViewModel.startButtonState)
    presentationStepsController.setStep(newViewModel.presentationStep)

    estimatesPreview.onNavigationInfoUpdated(viewModel)
    // TODO: update the navigation view
  }

  private func reloadRoutePoints(from oldPoints: RoutePreview.RoutePoints, to newPoints: RoutePreview.RoutePoints) {
    if newPoints.count < oldPoints.count {
      let indexToRemove = oldPoints.points.firstIndex { point in
        !newPoints.points.map(\.title).contains(point.title)
      }
      if let indexToRemove {
        routePointsCollectionView.deleteItems(at: [IndexPath(item: indexToRemove, section: 0)])
      }
    } else {
      routePointsCollectionView.reloadData()
    }
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

private extension RoutePreview.RoutePoints {
  func cellViewModel(for index: Int, onCloseHandler: (() -> Void)?) -> RouteStopCollectionViewCell.ViewModel {
    let point = self[index]
    return RouteStopCollectionViewCell.ViewModel(
      title: title(for: index),
      subtitle: subtitle(for: index),
      image: image(for: index),
      imageStyle: imageStyle(for: index),
      isPlaceholder: point == nil,
      onCloseHandler: onCloseHandler)
  }
}
