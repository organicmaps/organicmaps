final class RoutePointsView: UIView {

  private enum Constants {
    static let cellHeight: CGFloat = 44
    static var bottomContentInset: CGFloat {
      let bottomActionBartHeight = RouteActionsBottomMenuView.Constants.height +
                                   RouteActionsBottomMenuView.Constants.insets.top +
                                   RouteActionsBottomMenuView.Constants.insets.bottom
      let safeAreaInsets = MapsAppDelegate.theApp().window.safeAreaInsets
      return bottomActionBartHeight + safeAreaInsets.bottom + safeAreaInsets.top
    }
  }

  private var collectionView: UICollectionView!
  private var routePoints: NavigationDashboard.RoutePoints = .empty

  weak var interactor: NavigationDashboard.Interactor?
  weak var scrollViewDelegate: UIScrollViewDelegate?

  var contentBottom: CGPoint {
    origin.applying(CGAffineTransform(translationX: .zero, y: collectionView.contentSize.height))
  }

  init() {
    super.init(frame: .zero)
    setupView()
    layout()
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
    coordinator.animate(alongsideTransition: { _ in
      self.collectionView.collectionViewLayout.invalidateLayout()
    }, completion: nil)
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    updateCollectionViewInset()
  }

  private func updateCollectionViewInset() {
    collectionView.contentInset.bottom = Constants.bottomContentInset
  }

  private func setupView() {
    let layout = UICollectionViewFlowLayout()
    layout.scrollDirection = .vertical
    collectionView = UICollectionView(frame: .zero, collectionViewLayout: layout)
    collectionView.backgroundColor = .clear
    collectionView.dragInteractionEnabled = true
    collectionView.alwaysBounceVertical = false
    collectionView.isScrollEnabled = true
    collectionView.dragDelegate = self
    collectionView.dropDelegate = self
    collectionView.dataSource = self
    collectionView.delegate = self
    collectionView.register(cell: RoutePointCollectionViewCell.self)
    updateCollectionViewInset()
  }

  private func layout() {
    addSubview(collectionView)
    collectionView.translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      collectionView.leadingAnchor.constraint(equalTo: leadingAnchor),
      collectionView.trailingAnchor.constraint(equalTo: trailingAnchor),
      collectionView.topAnchor.constraint(equalTo: topAnchor),
      collectionView.bottomAnchor.constraint(equalTo: bottomAnchor)
    ])
  }

  private func configure(_ cell: RoutePointCollectionViewCell, at indexPath: IndexPath) {
    switch indexPath.item {
    case routePoints.count:
      cell.configure(with: .addPoint)
    default:
      let viewModel = routePoints.cellViewModel(for: indexPath.item, onCloseHandler: { [weak self] in
        if let point = self?.routePoints[indexPath.item] {
          self?.interactor?.process(.deleteRoutePoint(point))
        }
      })
      cell.configure(with: viewModel)
    }
  }

  func setRoutePoints(_ routePoints: NavigationDashboard.RoutePoints) {
    guard self.routePoints != routePoints else { return }
    self.routePoints = routePoints
    collectionView.reloadData()
  }
}

// MARK: - UIScrollViewDelegate
extension RoutePointsView: UIScrollViewDelegate {
  func scrollViewDidScroll(_ scrollView: UIScrollView) {
    scrollViewDelegate?.scrollViewDidScroll?(scrollView)
  }
}

// MARK: - UICollectionViewDataSource, UICollectionViewDelegate
extension RoutePointsView: UICollectionViewDataSource, UICollectionViewDelegate {
  func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
    routePoints.hasStartAndFinish ? routePoints.count + 1 : routePoints.count
  }

  func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
    let cell = collectionView.dequeueReusableCell(cell: RoutePointCollectionViewCell.self, indexPath: indexPath)
    configure(cell, at: indexPath)
    return cell
  }

  func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    switch indexPath.item {
    case routePoints.count:
      interactor?.process(.addRoutePointButtonDidTap)
    default:
      interactor?.process(.selectRoutePoint(routePoints[indexPath.item]))
    }
  }
}

// MARK: - UICollectionViewDelegateFlowLayout
extension RoutePointsView: UICollectionViewDelegateFlowLayout {
  func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, sizeForItemAt indexPath: IndexPath) -> CGSize {
    CGSize(width: collectionView.bounds.width, height: Constants.cellHeight)
  }

  func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, minimumLineSpacingForSectionAt section: Int) -> CGFloat {
    0
  }
}

// MARK: - UICollectionViewDragDelegate, UICollectionViewDropDelegate
extension RoutePointsView: UICollectionViewDragDelegate, UICollectionViewDropDelegate {
  func collectionView(_ collectionView: UICollectionView, itemsForBeginning session: UIDragSession, at indexPath: IndexPath) -> [UIDragItem] {
    guard indexPath.item < routePoints.count else { return [] }
    let item = routePoints[indexPath.item]
    let title = routePoints.title(for: indexPath.item)
    let itemProvider = NSItemProvider(object: title as NSString)
    let dragItem = UIDragItem(itemProvider: itemProvider)
    dragItem.localObject = item
    return [dragItem]
  }

  func collectionView(_ collectionView: UICollectionView, dragPreviewParametersForItemAt indexPath: IndexPath) -> UIDragPreviewParameters? {
    let parameters = UIDragPreviewParameters()
    parameters.backgroundColor = .clear
    return parameters
  }

  func collectionView(_ collectionView: UICollectionView, performDropWith coordinator: UICollectionViewDropCoordinator) {
    guard let destinationIndexPath = coordinator.destinationIndexPath,
          destinationIndexPath.row < routePoints.count else { return }

    for item in coordinator.items {
      if let sourceIndexPath = item.sourceIndexPath {
        guard sourceIndexPath != destinationIndexPath,
              sourceIndexPath.row < routePoints.count else {
          return
        }

        routePoints.movePoint(from: sourceIndexPath.row, to: destinationIndexPath.row)
        collectionView.performBatchUpdates {
          collectionView.moveItem(at: sourceIndexPath, to: destinationIndexPath)
        }
        for i in 0..<routePoints.count {
          let indexPath = IndexPath(item: i, section: 0)
          guard let cell = collectionView.cellForItem(at: indexPath) as? RoutePointCollectionViewCell else { continue }
          configure(cell, at: indexPath)
        }
        coordinator.drop(item.dragItem, toItemAt: destinationIndexPath)
        if !routePoints.hasStartAndFinish {
          interactor?.process(.swapStartAndFinishPoints)
        } else {
          interactor?.process(.moveRoutePoint(from: sourceIndexPath.row, to: destinationIndexPath.row))
        }
      }
    }
  }

  func collectionView(_ collectionView: UICollectionView, canHandle session: UIDropSession) -> Bool {
    true
  }

  func collectionView(_ collectionView: UICollectionView, dropSessionDidUpdate session: UIDropSession, withDestinationIndexPath destinationIndexPath: IndexPath?) -> UICollectionViewDropProposal {
    guard let destinationIndexPath = destinationIndexPath,
          destinationIndexPath.item < routePoints.count else {
      return UICollectionViewDropProposal(operation: .forbidden)
    }
    return UICollectionViewDropProposal(operation: .move, intent: .insertAtDestinationIndexPath)
  }

  func collectionView(_ collectionView: UICollectionView, dropPreviewParametersForItemAt indexPath: IndexPath) -> UIDragPreviewParameters? {
    let parameters = UIDragPreviewParameters()
    parameters.backgroundColor = .clear
    return parameters
  }
}

private extension NavigationDashboard.RoutePoints {
  func cellViewModel(for index: Int, onCloseHandler: (() -> Void)?) -> RoutePointCollectionViewCell.CellType {
    let point = self[index]
    let maskedCorners: CACornerMask
    switch index {
    case 0:
      maskedCorners = [.layerMinXMinYCorner, .layerMaxXMinYCorner]
    default:
      maskedCorners = hasStartAndFinish ? [] : [.layerMinXMaxYCorner, .layerMaxXMaxYCorner]
    }
    let viewModel = RoutePointCollectionViewCell.PointViewModel(
      title: title(for: index),
      image: image(for: index),
      showCloseButton: point?.type == .intermediate,
      maskedCorners: maskedCorners,
      isPlaceholder: point == nil,
      showSeparator: hasStartAndFinish ? true : index < points.count,
      onCloseHandler: onCloseHandler
    )
    return .point(viewModel)
  }
}
