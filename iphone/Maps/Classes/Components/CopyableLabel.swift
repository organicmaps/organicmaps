// Label shows copy popup menu on tap or long tap.
class CopyableLabel: UILabel {
  override init(frame: CGRect) {
    super.init(frame: frame)
    sharedInit()
  }

  required init?(coder aDecoder: NSCoder) {
    super.init(coder: aDecoder)
    sharedInit()
  }

  private func sharedInit() {
    isUserInteractionEnabled = true
    gestureRecognizers = [
      UILongPressGestureRecognizer(target: self, action: #selector(showMenu)),
      UITapGestureRecognizer(target: self, action: #selector(showMenu)),
    ]
  }

  @objc func showMenu(_ recognizer: UILongPressGestureRecognizer) {
    becomeFirstResponder()

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

  override func copy(_: Any?) {
    UIPasteboard.general.string = text
    UIMenuController.shared.setMenuVisible(false, animated: true)
  }

  override var canBecomeFirstResponder: Bool {
    true
  }

  override func canPerformAction(_ action: Selector, withSender _: Any?) -> Bool {
    action == #selector(UIResponderStandardEditActions.copy)
  }
}
