/// Full-screen per-leg breakdown of a public transport route, opened from the transit steps strip in the
/// route preview: where to board, transfer and exit, the line of every ride and the stops in between.
final class TransitDetailsViewController: MWMViewController {
  private enum Constants {
    static let horizontalInset: CGFloat = 16
    static let verticalInset: CGFloat = 12
    static let rowSpacing: CGFloat = 12
  }

  private let rows: [TransitDetailsRow]
  private let scrollView = UIScrollView()
  private let stackView = UIStackView()

  init(steps: [MWMRouterTransitStepInfo], routePoints: [MWMRoutePoint]) {
    let destination = routePoints.last { $0.type == .finish }?.title
    var waypointNames: [Int: String] = [:]
    for point in routePoints where point.type == .intermediate && Self.hasName(point) {
      waypointNames[point.intermediateIndex] = point.title
    }
    rows = TransitDetailsRows.make(steps: steps, destination: destination, waypointNames: waypointNames)
    super.init(nibName: nil, bundle: nil)
  }

  /// An unnamed point's title is its formatted coordinates; such a stop is shown by its number instead.
  static func hasName(_ point: MWMRoutePoint) -> Bool {
    point.isMyPosition || (!point.title.isEmpty && point.title != point.latLonString)
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    title = L("transit_details_title")
    view.setStyle(.background)

    stackView.axis = .vertical
    stackView.spacing = Constants.rowSpacing
    stackView.isLayoutMarginsRelativeArrangement = true
    stackView.directionalLayoutMargins = NSDirectionalEdgeInsets(top: Constants.verticalInset,
                                                                 leading: Constants.horizontalInset,
                                                                 bottom: Constants.verticalInset,
                                                                 trailing: Constants.horizontalInset)
    for row in rows {
      let rowView = view(for: row)
      stackView.addArrangedSubview(rowView)
      // A line running on into the next ride's transfer heading stays unbroken up to that heading.
      if case .ride(let ride) = row, ride.continuesIntoTransfer {
        stackView.setCustomSpacing(0, after: rowView)
      }
    }

    view.addSubview(scrollView)
    scrollView.addSubview(stackView)
    scrollView.translatesAutoresizingMaskIntoConstraints = false
    stackView.translatesAutoresizingMaskIntoConstraints = false
    NSLayoutConstraint.activate([
      scrollView.topAnchor.constraint(equalTo: view.safeAreaLayoutGuide.topAnchor),
      scrollView.leadingAnchor.constraint(equalTo: view.safeAreaLayoutGuide.leadingAnchor),
      scrollView.trailingAnchor.constraint(equalTo: view.safeAreaLayoutGuide.trailingAnchor),
      scrollView.bottomAnchor.constraint(equalTo: view.bottomAnchor),
      stackView.topAnchor.constraint(equalTo: scrollView.contentLayoutGuide.topAnchor),
      stackView.leadingAnchor.constraint(equalTo: scrollView.contentLayoutGuide.leadingAnchor),
      stackView.trailingAnchor.constraint(equalTo: scrollView.contentLayoutGuide.trailingAnchor),
      stackView.bottomAnchor.constraint(equalTo: scrollView.contentLayoutGuide.bottomAnchor),
      stackView.widthAnchor.constraint(equalTo: scrollView.frameLayoutGuide.widthAnchor),
    ])
  }

  private func view(for row: TransitDetailsRow) -> UIView {
    switch row {
    case .walk(let step): TransitDetailsWalkView(step: step)
    case .ride(let ride): TransitDetailsRideView(ride: ride)
    case .waypoint(let index, let name): TransitDetailsPointView(waypointIndex: index, name: name)
    case .destination(let name): TransitDetailsPointView(destination: name)
    }
  }
}
