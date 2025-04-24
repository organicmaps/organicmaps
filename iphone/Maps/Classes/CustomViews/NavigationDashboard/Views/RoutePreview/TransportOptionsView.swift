final class TransportOptionsView: UIView {

  private enum Constants {
    static let transportOptionsItemSize = CGSize(width: 40, height: 40)
  }

  private var collectionView: UICollectionView!
  private var transportOptions: [MWMRouterType] = []
  private var selectedRouterType: MWMRouterType = .vehicle

  weak var interactor: RoutePreview.Interactor?

  init() {
    super.init(frame: .zero)
    setupView()
    layout()
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  private func setupView() {
    let layout = UICollectionViewFlowLayout()
    layout.scrollDirection = .horizontal
    collectionView = UICollectionView(frame: .zero, collectionViewLayout: layout)
    collectionView.backgroundColor = .clear
    collectionView.dataSource = self
    collectionView.delegate = self
    collectionView.showsHorizontalScrollIndicator = false
    collectionView.showsVerticalScrollIndicator = false
    collectionView.isScrollEnabled = false
    collectionView.allowsMultipleSelection = false
    collectionView.register(cell: TransportOptionCollectionViewCell.self)
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

  func set(transportOptions: [MWMRouterType], selectedRouterType: MWMRouterType) {
    guard self.transportOptions != transportOptions || self.selectedRouterType != selectedRouterType else {
      return
    }
    self.transportOptions = transportOptions
    self.selectedRouterType = selectedRouterType
    reload()
  }

  func reload() {
    collectionView.reloadData()
  }
}

// MARK: - UICollectionViewDataSource, UICollectionViewDelegate
extension TransportOptionsView: UICollectionViewDataSource, UICollectionViewDelegate {
  func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
    transportOptions.count
  }

  func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
    let cell = collectionView.dequeueReusableCell(cell: TransportOptionCollectionViewCell.self, indexPath: indexPath)
    let routerType = transportOptions[indexPath.item]
    cell.configure(with: routerType)
    if self.selectedRouterType == routerType {
      collectionView.selectItem(at: indexPath, animated: true, scrollPosition: [])
    }
    return cell
  }

  func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    let routerType = transportOptions[indexPath.item]
    interactor?.process(.selectRouterType(routerType))
  }
}

// MARK: - UICollectionViewDelegateFlowLayout
extension TransportOptionsView: UICollectionViewDelegateFlowLayout {
  func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, sizeForItemAt indexPath: IndexPath) -> CGSize {
    Constants.transportOptionsItemSize
  }

  func collectionView(_ collectionView: UICollectionView, layout collectionViewLayout: UICollectionViewLayout, minimumLineSpacingForSectionAt section: Int) -> CGFloat {
    let cellsCount = CGFloat(transportOptions.count)
    let size = (collectionView.width - Constants.transportOptionsItemSize.width * cellsCount) / (cellsCount - 1)
    return size
  }
}
