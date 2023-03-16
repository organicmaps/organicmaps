// Label shows copy popup menu on tap or long tap.
class CopyableLabel: UILabel {

  override init(frame: CGRect) {
    super.init(frame: frame)
    self.sharedInit()
  }

  required init?(coder aDecoder: NSCoder) {
    super.init(coder: aDecoder)
    self.sharedInit()
  }

  private func sharedInit() {
    self.isUserInteractionEnabled = true
    self.gestureRecognizers = [
        UILongPressGestureRecognizer(target: self, action: #selector(self.showMenu)),
        UITapGestureRecognizer(target: self, action: #selector(self.showMenu))
    ]
  }

  @objc func showMenu(_ recognizer: UILongPressGestureRecognizer) {
    self.becomeFirstResponder()

    let menu = UIMenuController.shared
    let locationOfTouchInLabel = recognizer.location(in: self)

    if !menu.isMenuVisible {
      var rect = bounds
      rect.origin = locationOfTouchInLabel
      rect.size = CGSize(width: 1, height: 1)
      if #available(iOS 13, *) {
        menu.showMenu(from: self, rect: rect)
      } else {
        menu.setTargetRect(rect, in: self)
        menu.setMenuVisible(true, animated: false)
      }
    }
  }

  override func copy(_ sender: Any?) {
    UIPasteboard.general.string = text
    UIMenuController.shared.setMenuVisible(false, animated: true)
  }

  override var canBecomeFirstResponder: Bool {
    return true
  }

  override func canPerformAction(_ action: Selector, withSender sender: Any?) -> Bool {
    return action == #selector(UIResponderStandardEditActions.copy)
  }
}
