final class PopoverListSelectorViewController: UITableViewController, UIPopoverPresentationControllerDelegate {
  private enum Constants {
    static let rowHeight: CGFloat = 48
    static let maxPopoverWidth: CGFloat = 500
    static let horizontalScreenInset: CGFloat = 12
    static let backgroundColorAlpha: CGFloat = 0.25
    static let cellVerticalInset: CGFloat = 2.0
    static let iconSize: CGFloat = 14
  }

  enum Style {
    case background
    case icon
  }

  struct RowViewModel {
    enum Title {
      case string(String)
      case attributed(NSAttributedString)
    }

    let title: Title
    let color: UIColor?
    let isSelected: Bool
    let selectionHandler: () -> Void
  }

  private let dataSource: [RowViewModel]
  private let style: Style

  init(_ dataSource: [RowViewModel], style: Style = .background) {
    self.dataSource = dataSource
    self.style = style
    super.init(style: .plain)
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    tableView.setStyle(.clearBackground)
    tableView.register(cell: UITableViewCell.self)
    tableView.rowHeight = UITableView.automaticDimension
    tableView.estimatedRowHeight = Constants.rowHeight
    tableView.separatorStyle = .none
    updatePreferredContentSize()
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    updatePreferredContentSize()
  }

  override func tableView(_: UITableView, numberOfRowsInSection _: Int) -> Int {
    dataSource.count
  }

  override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let cell = tableView.dequeueReusableCell(cell: UITableViewCell.self, indexPath: indexPath)
    configureCell(cell, with: dataSource[indexPath.row])
    return cell
  }

  override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    tableView.deselectRow(at: indexPath, animated: true)
    dataSource[indexPath.row].selectionHandler()
  }

  func adaptivePresentationStyle(for _: UIPresentationController) -> UIModalPresentationStyle {
    .none
  }

  private var popoverWidth: CGFloat {
    guard let screenWidth = UIApplication.shared.activeKeyWindow?.frame.width else {
      return Constants.maxPopoverWidth
    }
    return min(screenWidth - Constants.horizontalScreenInset * 2, Constants.maxPopoverWidth)
  }

  private func configureCell(_ cell: UITableViewCell, with row: RowViewModel) {
    var content = cell.defaultContentConfiguration()
    switch row.title {
    case .string(let title):
      content.text = title
      content.textProperties.font = UIFont.regular14.dynamic
      content.textProperties.color = UIColor.blackPrimaryText
    case .attributed(let title):
      content.attributedText = title
    }
    content.textProperties.numberOfLines = 3
    content.directionalLayoutMargins.top = Constants.cellVerticalInset
    content.directionalLayoutMargins.bottom = Constants.cellVerticalInset
    if style == .icon, let iconColor = row.color {
      content.image = circleImageForColor(iconColor,
                                          frameSize: Constants.iconSize,
                                          diameter: Constants.iconSize)
    }

    cell.setStyle(.noStyleTableViewCell)
    cell.contentConfiguration = content
    cell.accessoryType = row.isSelected ? .checkmark : .none
    cell.tintColor = .linkBlue
    cell.backgroundColor = style == .background
      ? row.color?.withAlphaComponent(Constants.backgroundColorAlpha) ?? .clear
      : .clear
    cell.contentView.backgroundColor = .clear
  }

  private func updatePreferredContentSize() {
    tableView.layoutIfNeeded()
    let height = max(CGFloat(dataSource.count) * Constants.rowHeight, tableView.contentSize.height)
    preferredContentSize = CGSize(width: popoverWidth, height: height)
  }
}
