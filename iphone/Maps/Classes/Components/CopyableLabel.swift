/// Label shows copy popup menu on tap or long tap.
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
      menu.showMenu(from: self, rect: rect)
    }
  }

  override func copy(_: Any?) {
    UIPasteboard.general.string = text
    UIMenuController.shared.hideMenu(from: self)
  }

  override var canBecomeFirstResponder: Bool {
    true
  }

  override func canPerformAction(_ action: Selector, withSender _: Any?) -> Bool {
    action == #selector(UIResponderStandardEditActions.copy)
  }
}
