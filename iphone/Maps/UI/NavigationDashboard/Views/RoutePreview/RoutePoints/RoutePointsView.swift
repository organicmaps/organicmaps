final class RoutePointsView: UIView {
  private enum Constants {
    static var bottomContentInset: CGFloat {
      let safeAreaInsets = MapsAppDelegate.theApp().window?.safeAreaInsets ?? .zero
      let bottomActionBarHeight = RouteActionsBottomMenuView.reservedContentHeight(for: safeAreaInsets)
      return bottomActionBarHeight + safeAreaInsets.bottom + safeAreaInsets.top
    }
  }

  private var collectionView: UICollectionView!
  private var routePoints: NavigationDashboard.RoutePoints = .empty
  private var pendingTrackPoints: NavigationDashboard.RoutePoints?

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
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  func viewWillTransition(to _: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
    coordinator.animate(alongsideTransition: { _ in
      self.collectionView.collectionViewLayout.invalidateLayout()
    }, completion: nil)
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    updateCollectionViewInset()
    if previousTraitCollection?.preferredContentSizeCategory != traitCollection.preferredContentSizeCategory {
      collectionView.collectionViewLayout.invalidateLayout()
    }
  }

  override func safeAreaInsetsDidChange() {
    super.safeAreaInsetsDidChange()
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
      collectionView.bottomAnchor.constraint(equalTo: bottomAnchor),
    ])
  }

  private func configure(_ cell: RoutePointCollectionViewCell, at indexPath: IndexPath) {
    let index = indexPath.item
    let topConnector: RoutePointCollectionViewCell.ConnectorStyle = index == 0 ? .none :
      (index == routePoints.count ? .threeDots : .solid)
    let bottomConnector: RoutePointCollectionViewCell.ConnectorStyle
    if index < routePoints.count - 1 {
      bottomConnector = .solid
    } else if index == routePoints.count - 1, routePoints.hasStartAndFinish, routePoints.planningMode == .default {
      bottomConnector = .threeDots
    } else {
      bottomConnector = .none
    }
    switch indexPath.item {
    case routePoints.count:
      cell.configure(with: .addPoint, topConnector: topConnector, bottomConnector: bottomConnector)
    default:
      let viewModel = routePoints.cellViewModel(for: indexPath.item, onCloseHandler: { [weak self] in
        if let point = self?.routePoints[indexPath.item] {
          self?.interactor?.process(.deleteRoutePoint(point))
        }
      })
      cell.configure(with: viewModel, topConnector: topConnector, bottomConnector: bottomConnector)
    }
  }

  func setRoutePoints(_ routePoints: NavigationDashboard.RoutePoints) {
    if pendingTrackPoints != nil, routePoints.planningMode == .navigateTrack {
      pendingTrackPoints = routePoints
      return
    }
    pendingTrackPoints = nil
    guard self.routePoints != routePoints else { return }
    self.routePoints = routePoints
    collectionView.dragInteractionEnabled = (0 ..< routePoints.count).contains { routePoints.canMovePoint(at: $0) }
    collectionView.reloadData()
  }

  private func finishTrackDrop() {
    guard let points = pendingTrackPoints else { return }
    pendingTrackPoints = nil
    setRoutePoints(points)
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
  func collectionView(_: UICollectionView, numberOfItemsInSection _: Int) -> Int {
    routePoints.hasStartAndFinish && routePoints.planningMode == .default ? routePoints.count + 1 : routePoints.count
  }

  func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
    let cell = collectionView.dequeueReusableCell(cell: RoutePointCollectionViewCell.self, indexPath: indexPath)
    configure(cell, at: indexPath)
    return cell
  }

  func collectionView(_: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    guard routePoints.planningMode == .default else { return }
    switch indexPath.item {
    case routePoints.count:
      interactor?.process(.addRoutePointButtonDidTap)
    default:
      let selection = MWMRoutePointSelection(point: routePoints[indexPath.item],
                                             type: routePoints.type(for: indexPath.item),
                                             shouldAppend: false)
      interactor?.process(.selectRoutePoint(selection))
    }
  }
}

// MARK: - UICollectionViewDelegateFlowLayout

extension RoutePointsView: UICollectionViewDelegateFlowLayout {
  func collectionView(_ collectionView: UICollectionView, layout _: UICollectionViewLayout, sizeForItemAt _: IndexPath) -> CGSize {
    CGSize(width: collectionView.bounds.width, height: RoutePointCollectionViewCell.height())
  }

  func collectionView(_: UICollectionView, layout _: UICollectionViewLayout, minimumLineSpacingForSectionAt _: Int) -> CGFloat {
    0
  }
}

// MARK: - UICollectionViewDragDelegate, UICollectionViewDropDelegate

extension RoutePointsView: UICollectionViewDragDelegate, UICollectionViewDropDelegate {
  func collectionView(_: UICollectionView, itemsForBeginning _: UIDragSession, at indexPath: IndexPath) -> [UIDragItem] {
    guard routePoints.canMovePoint(at: indexPath.item), pendingTrackPoints == nil else { return [] }
    let item = routePoints[indexPath.item]
    let title = routePoints.title(for: indexPath.item)
    let itemProvider = NSItemProvider(object: title as NSString)
    let dragItem = UIDragItem(itemProvider: itemProvider)
    dragItem.localObject = item
    return [dragItem]
  }

  func collectionView(_: UICollectionView, dragPreviewParametersForItemAt _: IndexPath) -> UIDragPreviewParameters? {
    let parameters = UIDragPreviewParameters()
    parameters.backgroundColor = .clear
    return parameters
  }

  func collectionView(_ collectionView: UICollectionView, performDropWith coordinator: UICollectionViewDropCoordinator) {
    guard let destinationIndexPath = coordinator.destinationIndexPath else { return }

    for item in coordinator.items {
      if let sourceIndexPath = item.sourceIndexPath {
        guard routePoints.canMovePoint(from: sourceIndexPath.item, to: destinationIndexPath.item) else { return }

        routePoints.movePoint(from: sourceIndexPath.row, to: destinationIndexPath.row)
        if routePoints.planningMode == .navigateTrack {
          pendingTrackPoints = routePoints
        }
        collectionView.performBatchUpdates {
          collectionView.moveItem(at: sourceIndexPath, to: destinationIndexPath)
        }
        for i in 0 ..< routePoints.count {
          let indexPath = IndexPath(item: i, section: 0)
          guard let cell = collectionView.cellForItem(at: indexPath) as? RoutePointCollectionViewCell else { continue }
          configure(cell, at: indexPath)
        }
        let animation = coordinator.drop(item.dragItem, toItemAt: destinationIndexPath)
        if routePoints.planningMode == .navigateTrack {
          animation.addCompletion { [weak self] _ in self?.finishTrackDrop() }
          interactor?.process(.reverseRoutePoints)
        } else {
          interactor?.process(.moveRoutePoint(from: sourceIndexPath.row, to: destinationIndexPath.row))
        }
      }
    }
  }

  func collectionView(_: UICollectionView, canHandle session: UIDropSession) -> Bool {
    routePoints.planningMode == .default || (routePoints.count == 3 && session.localDragSession != nil)
  }

  func collectionView(_: UICollectionView, dropSessionDidUpdate session: UIDropSession, withDestinationIndexPath destinationIndexPath: IndexPath?) -> UICollectionViewDropProposal {
    guard let destinationIndexPath, destinationIndexPath.item < routePoints.count else {
      return UICollectionViewDropProposal(operation: .forbidden)
    }
    if routePoints.planningMode == .navigateTrack {
      guard let point = session.localDragSession?.items.first?.localObject as? MWMRoutePoint,
            let source = routePoints.points.firstIndex(of: point),
            routePoints.canMovePoint(from: source, to: destinationIndexPath.item) else {
        return UICollectionViewDropProposal(operation: .forbidden)
      }
    }
    return UICollectionViewDropProposal(operation: .move, intent: .insertAtDestinationIndexPath)
  }

  func collectionView(_: UICollectionView, dropPreviewParametersForItemAt _: IndexPath) -> UIDragPreviewParameters? {
    let parameters = UIDragPreviewParameters()
    parameters.backgroundColor = .clear
    return parameters
  }
}

private extension NavigationDashboard.RoutePoints {
  func cellViewModel(for index: Int, onCloseHandler: (() -> Void)?) -> RoutePointCollectionViewCell.CellType {
    let point = self[index]
    let usesCurrentPositionIcon = index == 0 && point?.isMyPosition == true
    let maskedCorners: CACornerMask
    switch index {
    case 0:
      maskedCorners = [.layerMinXMinYCorner, .layerMaxXMinYCorner]
    case count - 1 where planningMode == .navigateTrack:
      maskedCorners = [.layerMinXMaxYCorner, .layerMaxXMaxYCorner]
    default:
      maskedCorners = hasStartAndFinish ? [] : [.layerMinXMaxYCorner, .layerMaxXMaxYCorner]
    }
    let viewModel = RoutePointCollectionViewCell.PointViewModel(
      title: title(for: index),
      image: usesCurrentPositionIcon ? UIImage.icCurrentPosition : defaultImage(for: index),
      usesCurrentPositionIcon: usesCurrentPositionIcon,
      showCloseButton: point != nil && planningMode == .default,
      maskedCorners: maskedCorners,
      isPlaceholder: point == nil,
      showSeparator: planningMode == .navigateTrack ? index < count - 1 : (hasStartAndFinish ? true : index < points.count),
      onCloseHandler: onCloseHandler,
      accessibilityDetail: planningMode == .navigateTrack ? point?.subtitle : nil,
      showReorderButton: canMovePoint(at: index)
    )
    return .point(viewModel)
  }
}
