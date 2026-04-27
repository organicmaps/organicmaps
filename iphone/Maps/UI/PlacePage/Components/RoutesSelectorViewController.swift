final class RoutesSelectorViewController: UITableViewController, UIPopoverPresentationControllerDelegate {
  private enum Constants {
    static let rowHeight: CGFloat = 48
    static let maxPopoverHeight: CGFloat = rowHeight * 12
    static let maxPopoverWidth: CGFloat = 500
    static let horizontalScreenInset: CGFloat = 12
    static let backgroundColorAlpha: CGFloat = 0.25
    static let cellVerticalInset: CGFloat = 2.0
  }

  private let routes: [PlacePageRoute]
  private let selectedRouteRelId: UInt32?
  private let routeSelectedHandler: (PlacePageRoute) -> Void

  init(routes: [PlacePageRoute],
       selectedRouteRelId: UInt32?,
       routeSelectedHandler: @escaping (PlacePageRoute) -> Void) {
    self.routes = routes
    self.selectedRouteRelId = selectedRouteRelId
    self.routeSelectedHandler = routeSelectedHandler
    super.init(style: .plain)
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    tableView.setStyle(.clearBackground)
    tableView.backgroundColor = .clear
    tableView.register(cell: UITableViewCell.self)
    tableView.rowHeight = Constants.rowHeight
    tableView.separatorStyle = .none
    tableView.tableFooterView = UIView()
    preferredContentSize = CGSize(width: popoverWidth,
                                  height: min(CGFloat(routes.count) * Constants.rowHeight, Constants.maxPopoverHeight))
  }

  override func tableView(_: UITableView, numberOfRowsInSection _: Int) -> Int {
    routes.count
  }

  override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let route = routes[indexPath.row]
    let cell = tableView.dequeueReusableCell(cell: UITableViewCell.self, indexPath: indexPath)
    configureCell(cell, with: route)
    return cell
  }

  override func tableView(_: UITableView, didSelectRowAt indexPath: IndexPath) {
    routeSelectedHandler(routes[indexPath.row])
  }

  func adaptivePresentationStyle(for _: UIPresentationController) -> UIModalPresentationStyle {
    .none
  }

  private var popoverWidth: CGFloat {
    let screenWidth = view.window?.windowScene?.screen.bounds.width ?? UIScreen.main.bounds.width
    return min(screenWidth - Constants.horizontalScreenInset * 2, Constants.maxPopoverWidth)
  }

  private func configureCell(_ cell: UITableViewCell, with route: PlacePageRoute) {
    var content = cell.defaultContentConfiguration()
    content.attributedText = routeMenuLabel(route)
    content.textProperties.numberOfLines = 2
    content.directionalLayoutMargins.top = Constants.cellVerticalInset
    content.directionalLayoutMargins.bottom = Constants.cellVerticalInset

    cell.setStyle(.customTableViewCell)
    cell.contentConfiguration = content
    cell.accessoryType = route.relId == selectedRouteRelId ? .checkmark : .none
    cell.tintColor = .linkBlue
    cell.backgroundColor = route.color?.withAlphaComponent(Constants.backgroundColorAlpha) ?? .clear
    cell.contentView.backgroundColor = .clear
  }

  private func routeMenuLabel(_ route: PlacePageRoute) -> NSAttributedString {
    let baseAttributes: [NSAttributedString.Key: Any] = [
      .font: UIFont.regular14(),
      .foregroundColor: UIColor.blackPrimaryText,
    ]
    let boldAttributes: [NSAttributedString.Key: Any] = [
      .font: UIFont.bold14(),
      .foregroundColor: UIColor.blackPrimaryText,
    ]
    let label = NSMutableAttributedString(string: route.ref, attributes: boldAttributes)
    if !route.from.isEmpty || !route.to.isEmpty {
      label.append(NSAttributedString(string: ": \(route.from)", attributes: baseAttributes))
      if !route.to.isEmpty {
        label.append(NSAttributedString(string: " → \(route.to)", attributes: baseAttributes))
      }
    }
    return label
  }
}
