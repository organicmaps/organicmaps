import UIKit

protocol BookmarkColorViewControllerDelegate: AnyObject {
  func bookmarkColorViewController(_ viewController: BookmarkColorViewController, didSelect color: BookmarkColor)
}

final class BookmarkColorViewController: MWMTableViewController {
  weak var delegate: BookmarkColorViewControllerDelegate?
  private let bookmarkColor: BookmarkColor

  private let colors: [BookmarkColor] = [.red, .pink, .purple, .deepPurple, .blue, .lightBlue, .cyan, .teal, .green,
                                         .lime, .yellow, .orange, .deepOrange, .brown, .gray, .blueGray]

  init(bookmarkColor: BookmarkColor) {
    self.bookmarkColor = bookmarkColor
    super.init(style: .grouped)
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    title = L("bookmark_color")
  }

  override func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
    colors.count
  }

  override func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
    let cell = tableView.dequeueDefaultCell(for: indexPath)
    let color = colors[indexPath.row]
    let selected = color == bookmarkColor
    cell.textLabel?.text = color.title
    cell.imageView?.image = color.image(selected)
    cell.accessoryType = selected ? .checkmark : .none
    return cell
  }

  override func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
    let color = colors[indexPath.row]
    delegate?.bookmarkColorViewController(self, didSelect: color)
    Statistics.logEvent(kStatEventName(kStatPlacePage, kStatChangeBookmarkColor),
                        withParameters: [kStatValue : color.title])
  }
}
