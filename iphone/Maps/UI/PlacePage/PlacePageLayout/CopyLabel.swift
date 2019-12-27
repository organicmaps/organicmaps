final class CopyLabel: UILabel {
  override var canBecomeFirstResponder: Bool {
    true
  }

  override func canPerformAction(_ action: Selector, withSender sender: Any?) -> Bool {
    action == #selector(copy(_:))
  }

  override func copy(_ sender: Any?) {
    UIPasteboard.general.string = text
  }
}
