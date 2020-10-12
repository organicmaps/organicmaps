final class BookmarksListSectionHeader: UITableViewHeaderFooterView {
  @IBOutlet var titleLabel: UILabel!
  @IBOutlet var visibilityButton: UIButton!

  typealias VisibilityHandlerClosure = () -> Void
  var visibilityHandler: VisibilityHandlerClosure?

  @IBAction func onVisibilityButton(_ sender: UIButton) {
    visibilityHandler?()
  }

  func config(_ section: IBookmarksListSectionViewModel) {
    titleLabel.text = section.sectionTitle
    switch section.visibilityButtonState {
    case .hidden:
      visibilityButton.isHidden = true
    case .hideAll:
      visibilityButton.isHidden = false
      visibilityButton.setTitle(L("bookmarks_groups_hide_all"), for: .normal)
    case .showAll:
      visibilityButton.isHidden = false
      visibilityButton.setTitle(L("bookmarks_groups_show_all"), for: .normal)
    }
  }
}
