import Chart

protocol ElevationProfileViewProtocol: AnyObject {
  var presenter: ElevationProfilePresenterProtocol? { get set }

  var userInteractionEnabled: Bool { get set }
  var isChartViewHidden: Bool { get set }
  var isChartViewInfoHidden: Bool { get set }
  var canReceiveUpdates: Bool { get }

  func setChartData(_ data: ChartPresentationData)
  func setActivePointDistance(_ distance: Double)
  func setMyPositionDistance(_ distance: Double)
  func reloadDescription()
}

final class ElevationProfileViewController: UIViewController {
  private enum Constants {
    static let chartViewInsets = UIEdgeInsets(top: 12, left: 16, bottom: 0, right: -16)
    static let chartViewHeight: CGFloat = 176
    static let chartViewPreviewHeight: CGFloat = 120
    static let descriptionCollectionViewHeight: CGFloat = 44
    static let graphViewContainerInsets = UIEdgeInsets(top: -4, left: 0, bottom: 0, right: 0)
  }

  enum PresentationStyle: Equatable {
    case track
    case routePreview

    fileprivate var chartHeight: CGFloat {
      switch self {
      case .track:
        Constants.chartViewHeight
      case .routePreview:
        Constants.chartViewPreviewHeight
      }
    }

    fileprivate var contentInsets: UIEdgeInsets {
      switch self {
      case .track:
        Constants.chartViewInsets
      case .routePreview:
        .zero
      }
    }

    fileprivate var forcedUserInteractionEnabled: Bool? {
      switch self {
      case .track:
        nil
      case .routePreview:
        false
      }
    }

    fileprivate var forcedChartViewInfoHidden: Bool? {
      switch self {
      case .track:
        nil
      case .routePreview:
        true
      }
    }
  }

  private var chartHeight: CGFloat = Constants.chartViewHeight
  private var contentInsets: UIEdgeInsets = Constants.chartViewInsets
  private var presentationStyle: PresentationStyle = .track
  private var forcedUserInteractionEnabled: Bool?
  private var forcedChartViewInfoHidden: Bool?

  var presenter: ElevationProfilePresenterProtocol?

  init(presentationStyle: PresentationStyle) {
    super.init(nibName: nil, bundle: nil)
    setPresentationStyle(presentationStyle)
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  private var chartView = ChartView()
  private var graphViewContainer = UIView()
  private var descriptionCollectionView: UICollectionView = {
    let layout = UICollectionViewFlowLayout()
    layout.scrollDirection = .horizontal
    layout.minimumInteritemSpacing = 0
    return UICollectionView(frame: .zero, collectionViewLayout: layout)
  }()

  private var chartViewBottomConstraint: NSLayoutConstraint!
  private var chartViewLeadingConstraint: NSLayoutConstraint!
  private var chartViewTrailingConstraint: NSLayoutConstraint!
  private var chartViewHeightConstraint: NSLayoutConstraint!
  private var descriptionCollectionViewTopConstraint: NSLayoutConstraint!
  private var descriptionCollectionViewLeadingConstraint: NSLayoutConstraint!
  private var descriptionCollectionViewTrailingConstraint: NSLayoutConstraint!

  // MARK: - Lifecycle

  override func viewDidLoad() {
    super.viewDidLoad()
    setupViews()
    layoutViews()
    applyPresentationOverrides()
    presenter?.configure()
  }

  override func viewWillLayoutSubviews() {
    super.viewWillLayoutSubviews()
    descriptionCollectionView.reloadData()
  }

  // MARK: - Private methods

  private func setupViews() {
    view.setStyle(.background)
    setupDescriptionCollectionView()
    setupChartView()
  }

  private func setupChartView() {
    graphViewContainer.translatesAutoresizingMaskIntoConstraints = false
    chartView.translatesAutoresizingMaskIntoConstraints = false
    chartView.onSelectedPointChanged = { [weak self] in
      self?.presenter?.onSelectedPointChanged($0)
    }
  }

  private func setupDescriptionCollectionView() {
    descriptionCollectionView.backgroundColor = .clear
    descriptionCollectionView.register(cell: ElevationProfileDescriptionCell.self)
    descriptionCollectionView.dataSource = presenter
    descriptionCollectionView.delegate = presenter
    descriptionCollectionView.isScrollEnabled = false
    descriptionCollectionView.translatesAutoresizingMaskIntoConstraints = false
    descriptionCollectionView.showsHorizontalScrollIndicator = false
    descriptionCollectionView.showsVerticalScrollIndicator = false
  }

  private func layoutViews() {
    view.addSubview(descriptionCollectionView)
    graphViewContainer.addSubview(chartView)
    view.addSubview(graphViewContainer)

    let descriptionView = descriptionCollectionView
    chartViewLeadingConstraint = chartView.leadingAnchor.constraint(equalTo: graphViewContainer.leadingAnchor,
                                                                    constant: contentInsets.left)
    chartViewTrailingConstraint = chartView.trailingAnchor.constraint(equalTo: graphViewContainer.trailingAnchor,
                                                                      constant: contentInsets.right)
    chartViewBottomConstraint = chartView.bottomAnchor.constraint(equalTo: graphViewContainer.bottomAnchor,
                                                                  constant: contentInsets.bottom)
    chartViewHeightConstraint = chartView.heightAnchor.constraint(equalToConstant: chartHeight)
    descriptionCollectionViewTopConstraint = descriptionView.topAnchor.constraint(
      equalTo: view.topAnchor,
      constant: contentInsets.top
    )
    descriptionCollectionViewLeadingConstraint = descriptionView.leadingAnchor.constraint(
      equalTo: view.leadingAnchor,
      constant: contentInsets.left
    )
    descriptionCollectionViewTrailingConstraint = descriptionView.trailingAnchor.constraint(
      equalTo: view.trailingAnchor,
      constant: contentInsets.right
    )
    NSLayoutConstraint.activate([
      descriptionCollectionViewTopConstraint,
      descriptionCollectionViewLeadingConstraint,
      descriptionCollectionViewTrailingConstraint,
      descriptionCollectionView.heightAnchor.constraint(equalToConstant: Constants.descriptionCollectionViewHeight),
      descriptionCollectionView.bottomAnchor.constraint(equalTo: graphViewContainer.topAnchor,
                                                        constant: Constants.graphViewContainerInsets.top),
      graphViewContainer.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      graphViewContainer.trailingAnchor.constraint(equalTo: view.trailingAnchor),
      graphViewContainer.bottomAnchor.constraint(equalTo: view.bottomAnchor),
      chartViewLeadingConstraint,
      chartViewTrailingConstraint,
      chartView.topAnchor.constraint(equalTo: graphViewContainer.topAnchor),
      chartViewBottomConstraint,
      chartViewHeightConstraint,
    ])
  }

  private func reconfigureLayout() {
    guard isViewLoaded else { return }
    let wasChartHidden = isChartViewHidden
    let wasChartViewInfoHidden = isChartViewInfoHidden
    let wasUserInteractionEnabled = userInteractionEnabled
    chartViewHeightConstraint.constant = wasChartHidden ? .zero : chartHeight
    chartViewLeadingConstraint.constant = contentInsets.left
    chartViewTrailingConstraint.constant = contentInsets.right
    chartViewBottomConstraint.constant = contentInsets.bottom
    descriptionCollectionViewTopConstraint.constant = contentInsets.top
    descriptionCollectionViewLeadingConstraint.constant = contentInsets.left
    descriptionCollectionViewTrailingConstraint.constant = contentInsets.right
    presenter?.configure()
    isChartViewHidden = wasChartHidden
    isChartViewInfoHidden = wasChartViewInfoHidden
    userInteractionEnabled = wasUserInteractionEnabled
    applyPresentationOverrides()
    reloadDescription()
    view.setNeedsLayout()
    view.layoutIfNeeded()
  }

  func setPresentationStyle(_ style: PresentationStyle) {
    guard presentationStyle != style else { return }
    presentationStyle = style
    forcedUserInteractionEnabled = style.forcedUserInteractionEnabled
    forcedChartViewInfoHidden = style.forcedChartViewInfoHidden
    configureChartLayout(height: style.chartHeight, insets: style.contentInsets)
    applyPresentationOverrides()
  }

  func configureChartLayout(height: CGFloat, insets: UIEdgeInsets) {
    let resolvedHeight = max(0, height)
    guard chartHeight != resolvedHeight || contentInsets != insets else { return }
    chartHeight = resolvedHeight
    contentInsets = insets
    reconfigureLayout()
  }

  private func applyPresentationOverrides() {
    if let forcedUserInteractionEnabled {
      chartView.isUserInteractionEnabled = forcedUserInteractionEnabled
    }
    if let forcedChartViewInfoHidden {
      chartView.isChartViewInfoHidden = forcedChartViewInfoHidden
    }
  }
}

// MARK: - ElevationProfileViewProtocol

extension ElevationProfileViewController: ElevationProfileViewProtocol {
  var userInteractionEnabled: Bool {
    get { chartView.isUserInteractionEnabled }
    set { chartView.isUserInteractionEnabled = forcedUserInteractionEnabled ?? newValue }
  }

  var isChartViewHidden: Bool {
    get { chartView.isHidden }
    set {
      chartView.isHidden = newValue
      graphViewContainer.isHidden = newValue
      chartViewHeightConstraint.constant = newValue ? .zero : chartHeight
    }
  }

  var isChartViewInfoHidden: Bool {
    get { chartView.isChartViewInfoHidden }
    set { chartView.isChartViewInfoHidden = forcedChartViewInfoHidden ?? newValue }
  }

  var canReceiveUpdates: Bool {
    chartView.chartData != nil
  }

  func setChartData(_ data: ChartPresentationData) {
    chartView.chartData = data
  }

  func setActivePointDistance(_ distance: Double) {
    chartView.setSelectedPoint(distance)
  }

  func setMyPositionDistance(_ distance: Double) {
    chartView.myPosition = distance
  }

  func reloadDescription() {
    descriptionCollectionView.reloadData()
  }
}
