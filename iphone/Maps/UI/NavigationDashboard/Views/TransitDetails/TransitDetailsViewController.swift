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

  init(rows: [TransitDetailsRow]) {
    self.rows = rows
    super.init(nibName: nil, bundle: nil)
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
      // The lines run on unbroken into a transfer row and out of it.
      if case .transfer = row, let previous = stackView.arrangedSubviews.last {
        stackView.setCustomSpacing(0, after: previous)
        stackView.addArrangedSubview(rowView)
        stackView.setCustomSpacing(0, after: rowView)
      } else {
        stackView.addArrangedSubview(rowView)
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
    case .walk(let walk): TransitDetailsWalkView(walk: walk)
    case .ride(let ride): TransitDetailsRideView(ride: ride)
    case .transfer(let stop, let from, let to): TransitDetailsTransferView(stop: stop, from: from, to: to)
    case .waypoint(let number, let title): TransitDetailsPointView(waypointNumber: number, title: title)
    case .destination(let name): TransitDetailsPointView(destination: name)
    }
  }
}
