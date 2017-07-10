@objc(MWMNavigationAddPointToastView)
final class NavigationAddPointToastView: UIView {
  @IBOutlet private var actionButton: UIButton! {
    didSet {
      actionButton.setTitleColor(UIColor.linkBlue(), for: .normal)
      actionButton.tintColor = UIColor.linkBlue()
      actionButton.titleLabel?.font = UIFont.regular17()
    }
  }
  @IBOutlet private var locationButton: UIButton! {
    didSet {
      locationButton.setTitle(L("button_use"), for: .normal)
      locationButton.setTitleColor(UIColor.linkBlue(), for: .normal)
      locationButton.tintColor = UIColor.linkBlue()
      locationButton.titleLabel?.font = UIFont.regular17()

      let flipTransform = CGAffineTransform(scaleX: -1, y: 1)
      locationButton.transform = flipTransform
      locationButton.titleLabel?.transform = flipTransform
      locationButton.imageView?.transform = flipTransform
    }
  }
  @IBOutlet private var locationButtonHidden: NSLayoutConstraint!

  func config(text: String, withLocationButton: Bool) {
    actionButton.setTitle(text, for: .normal)
    backgroundColor = UIColor.white()

    setNeedsLayout()
    if withLocationButton {
      locationButtonHidden.priority = UILayoutPriorityDefaultHigh
      locationButton.isHidden = false
    } else {
      locationButtonHidden.priority = UILayoutPriorityDefaultLow
      locationButton.isHidden = true
    }
    UIView.animate(withDuration: kDefaultAnimationDuration) { self.layoutIfNeeded() }
  }
}
