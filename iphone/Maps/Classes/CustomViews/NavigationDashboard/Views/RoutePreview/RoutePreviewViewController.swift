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

    static let transportOptionsCollectionInsets = UIEdgeInsets(top: 0, left: 16, bottom: 0, right: -16)
    static let transportOptionsCollectionHeight: CGFloat = 44
    static let transportOptionsItemSize = CGSize(width: 44, height: 44)
    static let transportOptionsItemSpacing: CGFloat = 16

    static let routePointsInsets = UIEdgeInsets(top: 8, left: 16, bottom: -8, right: -16)
    static let routePointsVerticalSpacing: CGFloat = 12
    static let routePointCellHeight: CGFloat = 52
    static let addRoutePointCellHeight: CGFloat = 30
    static let minRoutePointsCellsCount = 2

    static let etaLabelHeight: CGFloat = 30
    static let etaLabelTopSpacing: CGFloat = 8
    static let etaLabelLeadingPadding: CGFloat = 16

    static let startButtonBottomPadding: CGFloat = -16
    static let startButtonHeight: CGFloat = 44
  }

  // MARK: - UI Components
  private let grabberView = UIView()
  private let backButton = UIButton(type: .system)
  private var transportOptionsCollectionView: UICollectionView!
  private let etaLabel = UILabel()
  private let settingsButton = UIButton(type: .system)
  private var routePointsCollectionView: UICollectionView!
  private var routePointsCollectionHeightConstraint: NSLayoutConstraint!
  private let startButton = StartRouteButton()
  private let availableAreaView = SearchOnMapAreaView()

  var interactor: RoutePreview.Interactor?
  private var viewModel: RoutePreview.ViewModel = .initial
  private let presentationStepsController = ModalPresentationStepsController()


  private var routePointCellsCount: Int {
    max(viewModel.points.count, Constants.minRoutePointsCellsCount)
  }

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
    presentationStepsController.setInitialState()
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

  private func updateFrameOfPresentedViewInContainerView() {
    presentationStepsController.updateMaxAvailableFrame()
    availableAreaView.frame = presentationStepsController.currentFrame
    view.layoutIfNeeded()
  }

  // MARK: - Setup Views
  private func setupView() {
    view.backgroundColor = .clear
    availableAreaView.setStyle(.modalSheetBackground)
    setupGrabberView()
    setupBackButton()
    setupSettingsButton()
    setupEtaLabel()
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
    grabberView.setStyle(.grabber)
    // TODO: remove when the grabber will be the same on all the modal screens
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

  private func setupEtaLabel() {
    // TODO: apply style
//    etaLabel.text = ""
    etaLabel.font = UIFont.systemFont(ofSize: 14, weight: .bold)
  }

  private func setupStartButton() {
    startButton.addTarget(self, action: #selector(didTapStartButton), for: .touchUpInside)
  }

  // MARK: - Setup Collection View
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
    availableAreaView.addSubview(etaLabel)
    availableAreaView.addSubview(settingsButton)
    availableAreaView.addSubview(routePointsCollectionView)
    availableAreaView.addSubview(startButton)

    grabberView.translatesAutoresizingMaskIntoConstraints = false
    backButton.translatesAutoresizingMaskIntoConstraints = false
    transportOptionsCollectionView.translatesAutoresizingMaskIntoConstraints = false
    etaLabel.translatesAutoresizingMaskIntoConstraints = false
    settingsButton.translatesAutoresizingMaskIntoConstraints = false
    routePointsCollectionView.translatesAutoresizingMaskIntoConstraints = false
    startButton.translatesAutoresizingMaskIntoConstraints = false

    etaLabel.setContentHuggingPriority(.defaultHigh, for: .vertical)
    backButton.setContentHuggingPriority(.defaultHigh, for: .horizontal)
    settingsButton.setContentHuggingPriority(.defaultHigh, for: .horizontal)

    routePointsCollectionHeightConstraint = routePointsCollectionView.heightAnchor.constraint(equalToConstant: 0)

    NSLayoutConstraint.activate([
      grabberView.centerXAnchor.constraint(equalTo: availableAreaView.centerXAnchor),
      grabberView.widthAnchor.constraint(equalToConstant: Constants.grabberWidth),
      grabberView.topAnchor.constraint(equalTo: availableAreaView.topAnchor, constant: Constants.grabberTopInset),
      grabberView.heightAnchor.constraint(equalToConstant: Constants.grabberHeight),
      
      backButton.leadingAnchor.constraint(equalTo: availableAreaView.safeAreaLayoutGuide.leadingAnchor, constant: Constants.backButtonInsets.left),
      backButton.widthAnchor.constraint(equalToConstant: Constants.backButtonSize.width),
      backButton.topAnchor.constraint(equalTo: grabberView.bottomAnchor, constant: Constants.backButtonInsets.top),
      backButton.heightAnchor.constraint(equalToConstant: Constants.backButtonSize.height),

      transportOptionsCollectionView.leadingAnchor.constraint(equalTo: backButton.trailingAnchor, constant: Constants.transportOptionsCollectionInsets.left),
      transportOptionsCollectionView.trailingAnchor.constraint(equalTo: availableAreaView.trailingAnchor, constant: Constants.transportOptionsCollectionInsets.right),
      transportOptionsCollectionView.centerYAnchor.constraint(equalTo: backButton.centerYAnchor),
      transportOptionsCollectionView.heightAnchor.constraint(equalToConstant: Constants.transportOptionsCollectionHeight),

      etaLabel.leadingAnchor.constraint(equalTo: availableAreaView.leadingAnchor, constant: Constants.etaLabelLeadingPadding),
      etaLabel.heightAnchor.constraint(equalToConstant: Constants.etaLabelHeight),
      etaLabel.topAnchor.constraint(equalTo: transportOptionsCollectionView.bottomAnchor, constant: Constants.etaLabelTopSpacing),

      settingsButton.leadingAnchor.constraint(equalTo: etaLabel.trailingAnchor, constant: Constants.settingsButtonSpacing),
      settingsButton.trailingAnchor.constraint(equalTo: availableAreaView.trailingAnchor, constant: Constants.settingsButtonInsetRight),
      settingsButton.centerYAnchor.constraint(equalTo: etaLabel.centerYAnchor),
      settingsButton.heightAnchor.constraint(equalToConstant: Constants.settingsButtonSize),
      settingsButton.widthAnchor.constraint(equalTo: settingsButton.heightAnchor),

      routePointsCollectionView.leadingAnchor.constraint(equalTo: availableAreaView.leadingAnchor, constant: Constants.routePointsInsets.left),
      routePointsCollectionView.trailingAnchor.constraint(equalTo: availableAreaView.trailingAnchor, constant: Constants.routePointsInsets.right),
      routePointsCollectionView.topAnchor.constraint(equalTo: etaLabel.bottomAnchor, constant: Constants.routePointsInsets.top),
      routePointsCollectionView.bottomAnchor.constraint(equalTo: startButton.topAnchor, constant: Constants.routePointsInsets.bottom),
      routePointsCollectionHeightConstraint,

      startButton.leadingAnchor.constraint(equalTo: routePointsCollectionView.leadingAnchor),
      startButton.trailingAnchor.constraint(equalTo: routePointsCollectionView.trailingAnchor),
      startButton.heightAnchor.constraint(equalToConstant: Constants.startButtonHeight),
    ])
    updateRoutePointsCollectionConstraints()
  }

  private func updateRoutePointsCollectionConstraints() {
    let pointsCount = routePointCellsCount
    let height = CGFloat(pointsCount) * Constants.routePointCellHeight + Constants.addRoutePointCellHeight + Constants.routePointsVerticalSpacing * CGFloat(pointsCount)
    routePointsCollectionHeightConstraint.constant = height
  }

  private func movePoint(from sourceIndexPath: IndexPath, to destinationIndexPath: IndexPath, item: MWMRoutePoint) {
    guard sourceIndexPath.item != destinationIndexPath.item else { return }
    routePointsCollectionView.moveItem(at: sourceIndexPath, to: destinationIndexPath)
    updateRoutePointsCollectionConstraints()
  }

  // MARK: - Button Actions
  @objc private func didTapStartButton() {
    interactor?.process(.startNavigation)
  }

  @objc private func didTapBackButton() {
    interactor?.process(.close)
  }

  // MARK: - RoutePreviewView Protocol Methods
  func add(to parentViewController: UIViewController) {
    parentViewController.addChild(self)
    parentViewController.view.addSubview(view)
    didMove(toParent: parentViewController)
    view.frame = parentViewController.view.bounds
    view.autoresizingMask = [.flexibleHeight, .flexibleHeight]
  }
}

// MARK: - UICollectionViewDataSource, UICollectionViewDelegate
extension RoutePreviewViewController: UICollectionViewDataSource, UICollectionViewDelegate {
  func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
    switch collectionView {
    case transportOptionsCollectionView:
      return viewModel.transportOptions.count

    case routePointsCollectionView:
      return max(viewModel.points.count + 1, Constants.minRoutePointsCellsCount) // from + to + add point

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
      let pointsCount = viewModel.points.count
      let shouldShowAddPoint = indexPath.item == pointsCount &&
      pointsCount >= Constants.minRoutePointsCellsCount
      if shouldShowAddPoint {
        let cell = collectionView.dequeueReusableCell(cell: AddItemCollectionViewCell.self, indexPath: indexPath)
        cell.didTapAction = { [weak self] in
          guard let self else { return }
          // TODO: show point selection view
//          self.routePoints.append(Place(name: "New Place \(routePoints.count)", image: UIImage(resource: .icBad)))
//          self.routePointsCollectionView.insertItems(at: [IndexPath(item: self.routePoints.count - 1, section: 0)])
//          self.updateRoutePointsCollectionConstraints()
        }
        return cell
      } else {
        let cell = collectionView.dequeueReusableCell(cell: RouteStopCollectionViewCell.self, indexPath: indexPath)

        switch viewModel.points.count {
        case 0:
          cell.configurePlaceholder(for: indexPath.item == 0 ? .start : .finish)
        case 1:
          let point = viewModel.points[0]
          if (point.type == .start && indexPath.item == 0) ||
             (point.type == .finish && indexPath.item == 1) {
            cell.configure(with: point,
                           onCloseHandler: { [weak self] in
              self?.interactor?.process(.deleteRoutePoint(point))
            })
          } else {
            cell.configurePlaceholder(for: indexPath.item == 0 ? .start : .finish)
          }
        default:
          let point = viewModel.points[indexPath.item]
          cell.configure(with: point, onCloseHandler: { [weak self] in
            self?.interactor?.process(.deleteRoutePoint(point))
          })
        }
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
      break
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
      let isAddPointCell = indexPath.item == routePointCellsCount
      let height = isAddPointCell ? Constants.addRoutePointCellHeight : Constants.routePointCellHeight
      return CGSize(width: collectionView.bounds.width,
                    height: height)

    default:
      fatalError("Unknown collection view")
    }
  }

  func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, minimumInteritemSpacingForSectionAt section: Int) -> CGFloat {
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
    guard indexPath.item < viewModel.points.count else { return [] }
    let item = viewModel.points[indexPath.item]
    let itemProvider = NSItemProvider(object: item.title as NSString)
    let dragItem = UIDragItem(itemProvider: itemProvider)
    dragItem.localObject = item
    return [dragItem]
  }

  func collectionView(_ collectionView: UICollectionView, performDropWith coordinator: UICollectionViewDropCoordinator) {
    guard let destinationIndexPath = coordinator.destinationIndexPath else { return }

    for item in coordinator.items {
      if let sourceIndexPath = item.sourceIndexPath, let draggedItem = item.dragItem.localObject as? MWMRoutePoint {
        interactor?.process(.moveRoutePoint(from: sourceIndexPath.item, to: destinationIndexPath.item))
        movePoint(from: sourceIndexPath, to: destinationIndexPath, item: draggedItem)
        coordinator.drop(item.dragItem, toItemAt: destinationIndexPath)
      }
    }
  }

  func collectionView(_ collectionView: UICollectionView, canHandle session: UIDropSession) -> Bool {
    true
  }

  func collectionView(_ collectionView: UICollectionView, dropSessionDidUpdate session: UIDropSession, withDestinationIndexPath destinationIndexPath: IndexPath?) -> UICollectionViewDropProposal {
    UICollectionViewDropProposal(operation: .move, intent: .insertAtDestinationIndexPath)
  }
}

extension RoutePreviewViewController {
  func render(_ newViewModel: RoutePreview.ViewModel) {
    dump(newViewModel)
    guard !newViewModel.shouldClose else {
      close()
      return
    }

    var indexToRemove: Int?
    if newViewModel.points.count >= Constants.minRoutePointsCellsCount && newViewModel.points.count < viewModel.points.count {
      indexToRemove = viewModel.points.firstIndex { point in
        !newViewModel.points.map(\.title).contains(point.title)
      }
    }
    viewModel = newViewModel
    if let indexToRemove {
      routePointsCollectionView.deleteItems(at: [IndexPath(item: indexToRemove, section: 0)])
    } else {
      routePointsCollectionView.reloadData()
    }
    updateRoutePointsCollectionConstraints()
    startButton.setLoading(newViewModel.showActivityIndicator)
    startButton.isHidden = !newViewModel.shouldShowStartButton
    etaLabel.attributedText = viewModel.estimates
    presentationStepsController.setStep(newViewModel.presentationStep)
  }

  func close() {
    willMove(toParent: nil)
    presentationStepsController.close { [weak self] in
      self?.view.removeFromSuperview()
      self?.removeFromParent()
    }
  }
}


