final class BMCNotificationsHeader: UIView {
  @IBOutlet private weak var label: UILabel! {
    didSet {
      label.text = L("bookmark_lists").uppercased()
    }
  }
}
