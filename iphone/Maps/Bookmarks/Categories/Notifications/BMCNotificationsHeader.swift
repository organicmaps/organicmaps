final class BMCNotificationsHeader: UIView {
  @IBOutlet private weak var label: UILabel! {
    didSet {
      label.font = .medium14()
      label.textColor = .blackSecondaryText()
      label.text = L("bookmarks_groups").uppercased()
    }
  }
}
