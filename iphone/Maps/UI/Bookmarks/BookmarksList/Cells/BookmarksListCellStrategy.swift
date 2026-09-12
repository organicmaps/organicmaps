final class BookmarksListCellStrategy {
  /// The cell is passed instead of an index path because the row can be moved or deleted while the
  /// configured cell is alive.
  typealias EditHandlerClosure = (UITableViewCell) -> Void
  var cellEditHandler: EditHandlerClosure?

  func registerCells(_ tableView: UITableView) {
    tableView.register(cell: BookmarksListCell.self)
    tableView.registerNibForHeaderFooterView(BookmarksListSectionHeader.self)
  }

  func tableCell(_ tableView: UITableView,
                 for viewModel: IBookmarksListSectionViewModel,
                 at indexPath: IndexPath) -> UITableViewCell {
    switch viewModel {
    case let bookmarksSection as IBookmarksSectionViewModel:
      let bookmark = bookmarksSection.bookmarks[indexPath.row]
      let cell = tableView.dequeueReusableCell(cell: BookmarksListCell.self, indexPath: indexPath)
      cell.configure(.bookmark(bookmark, infoAction: { [weak self, weak cell] _ in
        guard let cell else { return }
        self?.cellEditHandler?(cell)
      }))
      return cell
    case let tracksSection as ITracksSectionViewModel:
      let track = tracksSection.tracks[indexPath.row]
      let cell = tableView.dequeueReusableCell(cell: BookmarksListCell.self, indexPath: indexPath)
      cell.configure(.track(track, infoAction: { [weak self, weak cell] _ in
        guard let cell else { return }
        self?.cellEditHandler?(cell)
      }))
      return cell
    default:
      fatalError("Unexpected item")
    }
  }

  func headerView(_ tableView: UITableView,
                  for viewModel: IBookmarksListSectionViewModel) -> UITableViewHeaderFooterView {
    let headerView = tableView.dequeueReusableHeaderFooterView(BookmarksListSectionHeader.self)
    headerView.config(viewModel)
    return headerView
  }
}
