final class RouteManagerFooterView: UIView {
  @IBOutlet private weak var cancelButton: UIButton!
  @IBOutlet private weak var planButton: UIButton!
  @IBOutlet weak var separator: UIView!
  @IBOutlet weak var background: UIView!
  var isPlanButtonEnabled = true {
    didSet {
      planButton.isEnabled = isPlanButtonEnabled
    }
  }
}
