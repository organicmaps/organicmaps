final class CopyLabel: UILabel {
  override var canBecomeFirstResponder: Bool {
    true
  }

  override func canPerformAction(_ action: Selector, withSender _: Any?) -> Bool {
    action == #selector(copy(_:))
  }

  override func copy(_: Any?) {
    UIPasteboard.general.string = text
  }
}
