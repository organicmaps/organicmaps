final class BMCNotificationsHeader: UIView {
  @IBOutlet private weak var label: UILabel! {
    didSet {
      label.text = L("bookmarks_groups").uppercased()
    }
  }
}
