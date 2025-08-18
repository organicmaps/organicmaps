final class BookmarksListCellStrategy {
  private enum CellId {
    static let listItem = "BookmarksListCell"
    static let subgroup = "SubgroupCell"
    static let sectionHeader = "SectionHeader"
  }

  typealias CheckHandlerClosure = (IBookmarksListSectionViewModel, Int, Bool) -> Void
  var cellCheckHandler: CheckHandlerClosure?

  typealias VisibilityHandlerClosure = (IBookmarksListSectionViewModel) -> Void
  var cellVisibilityHandler: VisibilityHandlerClosure?

  func registerCells(_ tableView: UITableView) {
    tableView.register(UINib(nibName: "BookmarksListCell", bundle: nil), forCellReuseIdentifier: CellId.listItem)
    tableView.register(UINib(nibName: "SubgroupCell", bundle: nil), forCellReuseIdentifier: CellId.subgroup)
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
      cell.config(bookmark)
      return cell
    case let tracksSection as ITracksSectionViewModel:
      let track = tracksSection.tracks[indexPath.row]
      let cell = tableView.dequeueReusableCell(withIdentifier: CellId.listItem, for: indexPath) as! BookmarksListCell
      cell.config(track)
      return cell
    case let subgroupsSection as ISubgroupsSectionViewModel:
      let subgroup = subgroupsSection.subgroups[indexPath.row]
      let cell = tableView.dequeueReusableCell(withIdentifier: CellId.subgroup, for: indexPath) as! SubgroupCell
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
