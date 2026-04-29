enum PlacePagePopupSelectorTitle {
  case string(String)
  case attributed(NSAttributedString)
}

struct PlacePagePopupSelectorRowViewModel {
  let title: PlacePagePopupSelectorTitle
  let backgroundColor: UIColor?
  let isSelected: Bool
  let selectionHandler: () -> Void
}

final class PlacePagePopupSelectorViewModel {
  private let rows: [PlacePagePopupSelectorRowViewModel]

  init(rows: [PlacePagePopupSelectorRowViewModel]) {
    self.rows = rows
  }

  var numberOfRows: Int {
    rows.count
  }

  func row(at indexPath: IndexPath) -> PlacePagePopupSelectorRowViewModel {
    rows[indexPath.row]
  }

  func selectRow(at indexPath: IndexPath) {
    rows[indexPath.row].selectionHandler()
  }
}

final class PopoverSelectorViewController: UITableViewController, UIPopoverPresentationControllerDelegate {
  private enum Constants {
    static let rowHeight: CGFloat = 48
    static let maxPopoverWidth: CGFloat = 500
    static let horizontalScreenInset: CGFloat = 12
    static let backgroundColorAlpha: CGFloat = 0.25
    static let cellVerticalInset: CGFloat = 2.0
  }

  private let dataSource: PlacePagePopupSelectorViewModel

  init(viewModel: PlacePagePopupSelectorViewModel) {
    dataSource = viewModel
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
    tableView.rowHeight = Constants.rowHeight
    tableView.separatorStyle = .none
    preferredContentSize = CGSize(width: popoverWidth,
                                  height: CGFloat(dataSource.numberOfRows) * Constants.rowHeight)
  }

  override func tableView(_: UITableView, numberOfRowsInSection _: Int) -> Int {
    dataSource.numberOfRows
  }

  override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let cell = tableView.dequeueReusableCell(cell: UITableViewCell.self, indexPath: indexPath)
    configureCell(cell, with: dataSource.row(at: indexPath))
    return cell
  }

  override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    tableView.deselectRow(at: indexPath, animated: true)
    dataSource.selectRow(at: indexPath)
  }

  func adaptivePresentationStyle(for _: UIPresentationController) -> UIModalPresentationStyle {
    .none
  }

  private var popoverWidth: CGFloat {
    let screenWidth = UIApplication.shared.activeKeyWindow?.frame.width ?? 0
    return min(screenWidth - Constants.horizontalScreenInset * 2, Constants.maxPopoverWidth)
  }

  private func configureCell(_ cell: UITableViewCell, with row: PlacePagePopupSelectorRowViewModel) {
    var content = cell.defaultContentConfiguration()
    switch row.title {
    case .string(let title):
      content.text = title
      content.textProperties.font = UIFont.regular14()
      content.textProperties.color = UIColor.blackPrimaryText
    case .attributed(let title):
      content.attributedText = title
    }
    content.textProperties.numberOfLines = 3
    content.directionalLayoutMargins.top = Constants.cellVerticalInset
    content.directionalLayoutMargins.bottom = Constants.cellVerticalInset

    cell.setStyle(.customTableViewCell)
    cell.contentConfiguration = content
    cell.accessoryType = row.isSelected ? .checkmark : .none
    cell.tintColor = .linkBlue
    cell.backgroundColor = row.backgroundColor?.withAlphaComponent(Constants.backgroundColorAlpha) ?? .clear
    cell.contentView.backgroundColor = .clear
  }
}
