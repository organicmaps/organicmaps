final class BookmarksListCellStrategy {
  private enum CellId {
    static let listItem = "BookmarksListCell"
    static let subgroup = "BookmarksListSubgroupCell"
    static let sectionHeader = "SectionHeader"
  }

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
    tableView.register(UINib(nibName: "BookmarksListSubgroupCell", bundle: nil), forCellReuseIdentifier: CellId.subgroup)
    tableView.register(UINib(nibName: "BookmarksListSectionHeader", bundle: nil),
                       forHeaderFooterViewReuseIdentifier: CellId.sectionHeader)
  }

  func tableCell(_ tableView: UITableView,
                 for viewModel: IBookmarksListSectionViewModel,
                 at indexPath: IndexPath) -> UITableViewCell {
    switch viewModel {
    case let bookmarksSection as IBookmarksSectionViewModel:
      let bookmark = bookmarksSection.bookmarks[indexPath.row]
      let cell = tableView.dequeueReusableCell(withIdentifier: CellId.listItem, for: indexPath) as! BookmarksListCell
      cell.configure(.bookmark(bookmark, infoAction: { [weak self, weak cell] _ in
        guard let cell else { return }
        self?.cellEditHandler?(cell)
      }))
      return cell
    case let tracksSection as ITracksSectionViewModel:
      let track = tracksSection.tracks[indexPath.row]
      let cell = tableView.dequeueReusableCell(withIdentifier: CellId.listItem, for: indexPath) as! BookmarksListCell
      cell.configure(.bookmark(track, infoAction: { [weak self, weak cell] _ in
        guard let cell else { return }
        self?.cellEditHandler?(cell)
      }))
      return cell
    case let subgroupsSection as ISubgroupsSectionViewModel:
      let subgroup = subgroupsSection.subgroups[indexPath.row]
      let cell = tableView.dequeueReusableCell(withIdentifier: CellId.subgroup, for: indexPath) as! BookmarksListSubgroupCell
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
    let headerView = tableView.dequeueReusableHeaderFooterView(withIdentifier: CellId.sectionHeader)
      as! BookmarksListSectionHeader
    headerView.config(viewModel)
    headerView.visibilityHandler = { [weak self] in
      self?.cellVisibilityHandler?(viewModel)
    }
    return headerView
  }
}
