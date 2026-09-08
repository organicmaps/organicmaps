final class BookmarksListCellStrategy {
  typealias CheckHandlerClosure = (IBookmarksListSectionViewModel, Int, Bool) -> Void
  var cellCheckHandler: CheckHandlerClosure?

  typealias VisibilityHandlerClosure = (IBookmarksListSectionViewModel) -> Void
  var cellVisibilityHandler: VisibilityHandlerClosure?

  /// The cell is passed instead of an index path because the row can be moved or deleted while the
  /// configured cell is alive.
  typealias EditHandlerClosure = (UITableViewCell) -> Void
  var cellEditHandler: EditHandlerClosure?

  func registerCells(_ tableView: UITableView) {
    tableView.register(cell: BookmarksListCell.self)
    tableView.registerNib(cell: BookmarksListSubgroupCell.self)
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
    case let subgroupsSection as ISubgroupsSectionViewModel:
      let subgroup = subgroupsSection.subgroups[indexPath.row]
      let cell = tableView.dequeueReusableCell(cell: BookmarksListSubgroupCell.self, indexPath: indexPath)
      cell.config(subgroup)
      cell.checkHandler = { [weak self] checked in
        self?.cellCheckHandler?(viewModel, indexPath.row, checked)
      }
      return cell
    default:
      fatalError("Unexpected item")
    }
  }

  func headerView(_ tableView: UITableView,
                  for viewModel: IBookmarksListSectionViewModel) -> UITableViewHeaderFooterView {
    let headerView = tableView.dequeueReusableHeaderFooterView(BookmarksListSectionHeader.self)
    headerView.config(viewModel)
    headerView.visibilityHandler = { [weak self] in
      self?.cellVisibilityHandler?(viewModel)
    }
    return headerView
  }
}
