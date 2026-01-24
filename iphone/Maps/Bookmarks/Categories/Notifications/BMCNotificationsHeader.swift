final class BMCNotificationsHeader: UIView {
  @IBOutlet private var label: UILabel! {
    didSet {
      label.text = L("bookmark_lists").uppercased()
    }
  }
}
