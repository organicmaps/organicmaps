@available(iOS 14.0, *)
@objcMembers
final class RoutePreviewViewController: UIViewController {

  // MARK: - RoutePreviewView Properties
  var drivingOptionsState: MWMDrivingOptionsState = .none
  weak var delegate: MWMRoutePreviewDelegate?

  // MARK: - UI Components
  private let mapView = UIView()
  private let transportOptionsStack = UIStackView()
  private let distanceLabel = UILabel()
  private let durationLabel = UILabel()
  private let settingsButton = UIButton(type: .system)
  private let startButton = UIButton(type: .system)
  private var collectionView: UICollectionView!

  // Data Source and Snapshots
  private var dataSource: UICollectionViewDiffableDataSource<Int, Place>!
  private var places: [Place] = []

  // MARK: - Lifecycle
  override func viewDidLoad() {
    super.viewDidLoad()
    setupView()
    setupCollectionView()
    layout()
    configureDataSource()
  }

  // MARK: - Setup Views
  private func setupView() {
    view.backgroundColor = .white
    setupMapView()
    setupTransportOptionsStack()
    setupInfoLabels()
    setupStartButton()
  }

  private func setupMapView() {
    mapView.backgroundColor = .systemGray5
    view.addSubview(mapView)
  }

  private func setupTransportOptionsStack() {
    let carButton = createTransportButton(imageName: "car.fill")
    let walkingButton = createTransportButton(imageName: "figure.walk")
    let busButton = createTransportButton(imageName: "bus")
    let bikeButton = createTransportButton(imageName: "bicycle")
    let customButton = createTransportButton(imageName: "slider.horizontal.3")

    transportOptionsStack.axis = .horizontal
    transportOptionsStack.distribution = .fillEqually
    transportOptionsStack.spacing = 8
    transportOptionsStack.addArrangedSubview(carButton)
    transportOptionsStack.addArrangedSubview(walkingButton)
    transportOptionsStack.addArrangedSubview(busButton)
    transportOptionsStack.addArrangedSubview(bikeButton)
    transportOptionsStack.addArrangedSubview(customButton)

    view.addSubview(transportOptionsStack)
  }

  private func setupInfoLabels() {
    distanceLabel.text = "32.4 KM"
    distanceLabel.font = UIFont.systemFont(ofSize: 14, weight: .bold)

    durationLabel.text = "1hr 21min"
    durationLabel.font = UIFont.systemFont(ofSize: 14, weight: .regular)

    view.addSubview(distanceLabel)
    view.addSubview(durationLabel)
  }

  private func setupStartButton() {
    startButton.setTitle("Start", for: .normal)
    startButton.backgroundColor = .systemBlue
    startButton.setTitleColor(.white, for: .normal)
    startButton.layer.cornerRadius = 10
    startButton.addTarget(self, action: #selector(didTapStartButton), for: .touchUpInside)
    view.addSubview(startButton)
  }

  // MARK: - Setup Collection View
  private func setupCollectionView() {
    let layout = createCompositionalLayout()
    collectionView = UICollectionView(frame: .zero, collectionViewLayout: layout)
    collectionView.backgroundColor = .white
    collectionView.dragInteractionEnabled = true
    collectionView.alwaysBounceVertical = false
    collectionView.isScrollEnabled = false
    collectionView.delegate = self
//    collectionView.dataSource = self
    collectionView.register(PlaceCell.self, forCellWithReuseIdentifier: PlaceCell.reuseIdentifier)
    view.addSubview(collectionView)
  }

  // MARK: - Layout Constraints
  private func layout() {
    mapView.translatesAutoresizingMaskIntoConstraints = false
    transportOptionsStack.translatesAutoresizingMaskIntoConstraints = false
    distanceLabel.translatesAutoresizingMaskIntoConstraints = false
    durationLabel.translatesAutoresizingMaskIntoConstraints = false
    collectionView.translatesAutoresizingMaskIntoConstraints = false
    startButton.translatesAutoresizingMaskIntoConstraints = false

    NSLayoutConstraint.activate([
      mapView.topAnchor.constraint(equalTo: view.topAnchor),
      mapView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      mapView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
      mapView.heightAnchor.constraint(equalToConstant: 200),

      transportOptionsStack.topAnchor.constraint(equalTo: mapView.bottomAnchor, constant: 8),
      transportOptionsStack.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: 16),
      transportOptionsStack.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -16),

      distanceLabel.topAnchor.constraint(equalTo: transportOptionsStack.bottomAnchor, constant: 8),
      distanceLabel.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: 16),

      durationLabel.centerYAnchor.constraint(equalTo: distanceLabel.centerYAnchor),
      durationLabel.leadingAnchor.constraint(equalTo: distanceLabel.trailingAnchor, constant: 8),

      collectionView.topAnchor.constraint(equalTo: distanceLabel.bottomAnchor, constant: 8),
      collectionView.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: 16),
      collectionView.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -16),
      collectionView.heightAnchor.constraint(equalToConstant: 200),

      startButton.topAnchor.constraint(equalTo: collectionView.bottomAnchor, constant: 16),
      startButton.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: 16),
      startButton.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -16),
      startButton.heightAnchor.constraint(equalToConstant: 50)
    ])
  }

  // MARK: - Collection View Layout
  private func createCompositionalLayout() -> UICollectionViewLayout {
    let itemSize = NSCollectionLayoutSize(widthDimension: .fractionalWidth(1), heightDimension: .absolute(60))
    let item = NSCollectionLayoutItem(layoutSize: itemSize)

    let group = NSCollectionLayoutGroup.vertical(layoutSize: itemSize, subitems: [item])

    let section = NSCollectionLayoutSection(group: group)
    section.interGroupSpacing = 8
    section.contentInsets = NSDirectionalEdgeInsets(top: 8, leading: 0, bottom: 8, trailing: 0)

    return UICollectionViewCompositionalLayout(section: section)
  }

  // MARK: - Data Source Configuration
  private func configureDataSource() {
    dataSource = UICollectionViewDiffableDataSource<Int, Place>(collectionView: collectionView) {
      (collectionView, indexPath, place) -> UICollectionViewCell? in
      guard let cell = collectionView.dequeueReusableCell(withReuseIdentifier: PlaceCell.reuseIdentifier, for: indexPath) as? PlaceCell else {
        return nil
      }
      cell.configure(with: place)
      return cell
    }

    dataSource.reorderingHandlers.canReorderItem = { _ in return true }
    dataSource.reorderingHandlers.didReorder = { [weak self] transaction in
      guard let self = self else { return }
      self.places = transaction.finalSnapshot.itemIdentifiers
      self.updateSnapshot()
    }
    updateSnapshot()
  }

  private func updateSnapshot() {
    var snapshot = NSDiffableDataSourceSnapshot<Int, Place>()
    snapshot.appendSections([0])
    snapshot.appendItems(places)
    dataSource.apply(snapshot, animatingDifferences: true)
  }

  // MARK: - Button Actions
  @objc private func didTapStartButton() {
    delegate?.routingStartButtonDidTap()
  }

  private func createTransportButton(imageName: String) -> UIButton {
    let button = UIButton(type: .system)
    button.setImage(UIImage(systemName: imageName), for: .normal)
    button.tintColor = .black
    return button
  }

  // MARK: - RoutePreviewView Protocol Methods
  func add(to superview: UIView) {
    superview.addSubview(self.view)
    self.view.translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      self.view.leadingAnchor.constraint(equalTo: superview.leadingAnchor),
      self.view.trailingAnchor.constraint(equalTo: superview.trailingAnchor),
      self.view.bottomAnchor.constraint(equalTo: superview.bottomAnchor),
      self.view.heightAnchor.constraint(equalToConstant: 500)
    ])
  }
//
//  func remove() {
//    view.removeFromSuperview()
//  }
//
//  func statePrepare() {
//    // Prepare state for preview
//  }
//
//  func select(_ routerType: MWMRouterType) {
//    // Select router logic
//  }
//
//  func router(_ routerType: MWMRouterType, setState state: MWMCircularProgressState) {
//    // Set state for router
//  }
//
//  func router(_ routerType: MWMRouterType, setProgress progress: CGFloat) {
//    // Set progress
//  }
}

// MARK: - Collection View Delegate for Drag/Drop
@available(iOS 14.0, *)
extension RoutePreviewViewController: UICollectionViewDelegate {
  func collectionView(_ collectionView: UICollectionView, moveItemAt sourceIndexPath: IndexPath, to destinationIndexPath: IndexPath) {
    let movedItem = places.remove(at: sourceIndexPath.item)
    places.insert(movedItem, at: destinationIndexPath.item)
    updateSnapshot()
  }
}

// MARK: - PlaceCell Implementation
class PlaceCell: UICollectionViewCell {
  static let reuseIdentifier = "PlaceCell"
  private let titleLabel = UILabel()

  override init(frame: CGRect) {
    super.init(frame: frame)
    setupCell()
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  private func setupCell() {
    titleLabel.font = UIFont.systemFont(ofSize: 14, weight: .regular)
    titleLabel.textAlignment = .left
    contentView.addSubview(titleLabel)
    titleLabel.translatesAutoresizingMaskIntoConstraints = false

    NSLayoutConstraint.activate([
      titleLabel.centerYAnchor.constraint(equalTo: contentView.centerYAnchor),
      titleLabel.leadingAnchor.constraint(equalTo: contentView.leadingAnchor, constant: 16)
    ])
  }

  func configure(with place: Place) {
    titleLabel.text = place.name
  }
}

// MARK: - Place Model
struct Place: Hashable {
  let id = UUID()
  let name: String
}

@available(iOS 14.0, *)
extension RoutePreviewViewController: NavigationDashboardView {
  func onNavigationInfoUpdated(_ entity: MWMNavigationDashboardEntity) {
    print(#function)
  }

  func setDrivingOptionState(_ state: MWMDrivingOptionsState) {
    print(#function)
  }

  func searchManager(withDidChange state: SearchOnMapState) {
    print(#function)
  }

  func updateNavigationInfoAvailableArea(_ frame: CGRect) {
    print(#function)
  }

  func setRouteBuilderProgress(_ router: MWMRouterType, progress: CGFloat) {
    print(#function)
  }

  func statePrepare() {
    print(#function)
  }

  func statePlanning() {
    print(#function)
  }

  func stateReady() {
    print(#function)
  }

  func onRouteStart() {
    print(#function)
  }

  func onRouteStop() {
    print(#function)
  }

  func onRoutePointsUpdated() {
    print(#function)
  }

  func stateNavigation() {
    print(#function)
  }

  func stateError(_ errorMessage: String) {
    print(#function)
  }

  func setHidden() {
    print(#function)
  }
}
