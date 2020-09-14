final class BookmarksListCellStrategy {
  private enum CellId {
    static let track = "TrackCell"
    static let bookmark = "BookmarkCell"
    static let sectionHeader = "SectionHeader"
  }

  func registerCells(_ tableView: UITableView) {
    tableView.register(UINib(nibName: "BookmarkCell", bundle: nil), forCellReuseIdentifier: CellId.bookmark)
    tableView.register(UINib(nibName: "TrackCell", bundle: nil), forCellReuseIdentifier: CellId.track)
    tableView.register(UINib(nibName: "BookmarksListSectionHeader", bundle: nil),
                       forHeaderFooterViewReuseIdentifier: CellId.sectionHeader)
  }

  func tableCell(_ tableView: UITableView,
                 for viewModel: IBookmarksListSectionViewModel,
                 at indexPath: IndexPath) -> UITableViewCell {
    switch viewModel {
    case let bookmarksSection as IBookmarksSectionViewModel:
      let bookmark = bookmarksSection.bookmarks[indexPath.row]
      let cell = tableView.dequeueReusableCell(withIdentifier: CellId.bookmark, for: indexPath) as! BookmarkCell
      cell.config(bookmark)
      return cell
    case let tracksSection as ITracksSectionViewModel:
      let track = tracksSection.tracks[indexPath.row]
      let cell = tableView.dequeueReusableCell(withIdentifier: CellId.track, for: indexPath) as! TrackCell
      cell.config(track)
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
    return headerView
  }
}
