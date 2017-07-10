@objc(MWMNavigationAddPointToastView)
final class NavigationAddPointToastView: UIView {
  @IBOutlet private var actionButton: UIButton! {
    didSet {
      actionButton.setTitleColor(UIColor.linkBlue(), for: .normal)
      actionButton.tintColor = UIColor.linkBlue()
      actionButton.titleLabel?.font = UIFont.regular16()
    }
  }
  @IBOutlet private var locationButton: UIButton! {
    didSet {
      locationButton.tintColor = UIColor.linkBlue()
    }
  }
  @IBOutlet private var separator: UIView! {
    didSet {
      separator.backgroundColor = UIColor.blackDividers()
    }
  }

  func config(text: String, withLocationButton: Bool) {
    actionButton.setTitle(text, for: .normal)
    backgroundColor = UIColor.white()

    if withLocationButton {
      locationButton.isHidden = false
      separator.isHidden = false
    } else {
      locationButton.isHidden = true
      separator.isHidden = true
    }
  }
}
