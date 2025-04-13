final class BookmarksListSectionHeader: UITableViewHeaderFooterView {
  @IBOutlet private var titleLabel: UILabel!
  @IBOutlet private var visibilityButton: UIButton!

  typealias VisibilityHandlerClosure = () -> Void
  var visibilityHandler: VisibilityHandlerClosure?

  @IBAction private func onVisibilityButton(_ sender: UIButton) {
    visibilityHandler?()
  }

  func config(_ section: IBookmarksListSectionViewModel) {
    titleLabel.text = section.sectionTitle
    switch section.visibilityButtonState {
    case .hidden:
      visibilityButton.isHidden = true
    case .hideAll:
      visibilityButton.isHidden = false
      visibilityButton.setTitle(L("bookmark_lists_hide_all"), for: .normal)
    case .showAll:
      visibilityButton.isHidden = false
      visibilityButton.setTitle(L("bookmark_lists_show_all"), for: .normal)
    }
  }
}
