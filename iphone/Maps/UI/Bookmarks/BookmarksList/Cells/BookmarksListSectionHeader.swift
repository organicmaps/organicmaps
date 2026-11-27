final class BookmarksListSectionHeader: UITableViewHeaderFooterView {
  @IBOutlet private var titleLabel: UILabel!

  override func awakeFromNib() {
    super.awakeFromNib()
    titleLabel.accessibilityTraits.insert(.header)
  }

  func config(_ section: IBookmarksListSectionViewModel) {
    titleLabel.text = section.sectionTitle
  }
}
