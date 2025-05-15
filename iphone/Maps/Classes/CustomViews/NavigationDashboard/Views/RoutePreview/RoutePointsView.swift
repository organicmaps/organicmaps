final class RoutePointsView: UIView {

  private enum Constants {
    static let routePointsInsets = UIEdgeInsets(top: 8, left: 0, bottom: -8, right: 0)
    static let routePointsVerticalSpacing: CGFloat = 6
    static let routePointCellHeight: CGFloat = 52
    static let addRoutePointCellHeight: CGFloat = 30
    static let bottomContentInset: CGFloat = 200
  }

  private var collectionView: UICollectionView!
  private var routePoints: RoutePreview.RoutePoints = .empty

  weak var interactor: RoutePreview.Interactor?

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
    collectionView.contentInset.bottom = Constants.bottomContentInset
    collectionView.register(cell: RouteStopCollectionViewCell.self)
    collectionView.register(cell: AddItemCollectionViewCell.self)
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

  func setRoutePoints(_ routePoints: RoutePreview.RoutePoints) {
    guard self.routePoints != routePoints else { return }
    self.routePoints = routePoints
    collectionView.reloadData()
  }
}

// MARK: - UICollectionViewDataSource, UICollectionViewDelegate
extension RoutePointsView: UICollectionViewDataSource, UICollectionViewDelegate {
  func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
    routePoints.count + 1
  }

  func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
    switch indexPath.item {
    case routePoints.count:
      let cell = collectionView.dequeueReusableCell(cell: AddItemCollectionViewCell.self, indexPath: indexPath)
      cell.didTapAction = { [weak self] in
        self?.interactor?.process(.addRoutePointButtonDidTap)
      }
      return cell
    default:
      let cell = collectionView.dequeueReusableCell(cell: RouteStopCollectionViewCell.self, indexPath: indexPath)
      let routePoints = routePoints
      cell.configure(with: routePoints.cellViewModel(for: indexPath.item, onCloseHandler: { [weak self] in
        if let point = routePoints[indexPath.item] {
          self?.interactor?.process(.deleteRoutePoint(point))
        }
      }))
      return cell
    }
  }

  func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    switch indexPath.item {
    case routePoints.count:
      break // Add point cell
    default:
      let point = routePoints[indexPath.item]
      interactor?.process(.selectRoutePoint(point))
      break
    }
  }
}

// MARK: - UICollectionViewDelegateFlowLayout
extension RoutePointsView: UICollectionViewDelegateFlowLayout {
  func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, sizeForItemAt indexPath: IndexPath) -> CGSize {
    let isAddPointCell = indexPath.item == routePoints.count
    let height = isAddPointCell ? Constants.addRoutePointCellHeight : Constants.routePointCellHeight
    return CGSize(width: collectionView.bounds.width,
                  height: height)
  }

  func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, minimumLineSpacingForSectionAt section: Int) -> CGFloat {
    Constants.routePointsVerticalSpacing
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

  func collectionView(_ collectionView: UICollectionView, performDropWith coordinator: UICollectionViewDropCoordinator) {
    guard let destinationIndexPath = coordinator.destinationIndexPath,
          destinationIndexPath.item < routePoints.count else { return }

    for item in coordinator.items {
      if let sourceIndexPath = item.sourceIndexPath, let _ = item.dragItem.localObject as? MWMRoutePoint {
        guard sourceIndexPath != destinationIndexPath else { return }
        interactor?.process(.moveRoutePoint(from: sourceIndexPath.item, to: destinationIndexPath.item))
        coordinator.drop(item.dragItem, toItemAt: destinationIndexPath)
        collectionView.reloadItems(at: [sourceIndexPath, destinationIndexPath])
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
