final class BookmarksListSectionHeader: UITableViewHeaderFooterView {
  @IBOutlet private var titleLabel: UILabel!

  func config(_ section: IBookmarksListSectionViewModel) {
    titleLabel.text = section.sectionTitle
  }
}
