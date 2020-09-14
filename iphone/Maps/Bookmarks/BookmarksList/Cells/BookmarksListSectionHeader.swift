final class BookmarksListSectionHeader: UITableViewHeaderFooterView {
  @IBOutlet var titleLabel: UILabel!
  @IBOutlet var visibilityButton: UIButton!

  @IBAction func onVisibilityButton(_ sender: UIButton) {

  }

  func config(_ section: IBookmarksListSectionViewModel) {
    titleLabel.text = section.sectionTitle
    visibilityButton.isHidden = !section.hasVisibilityButton
  }
}
