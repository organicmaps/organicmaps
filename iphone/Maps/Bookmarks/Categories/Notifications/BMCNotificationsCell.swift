final class BMCNotificationsCell: MWMTableViewCell {
  @IBOutlet private weak var spinner: UIView! {
    didSet {
      circularProgress = MWMCircularProgress(parentView: spinner)
      circularProgress.state = .spinner
    }
  }

  @IBOutlet private weak var label: UILabel! {
    didSet {
      label.text = L("load_kmz_title")
    }
  }

  private var circularProgress: MWMCircularProgress!
}
