@objc(MWMSearchTextField)
class SearchTextField: UITextField {
  required init?(coder aDecoder: NSCoder) {
    super.init(coder: aDecoder)
    leftViewMode = UITextField.ViewMode.always
    leftView = UIView(frame: CGRect(x: 0, y: 0, width: 32, height: 32))
  }

  override func drawPlaceholder(in rect: CGRect) {
    guard let font = font, let tint = tintColor else {
      super.drawPlaceholder(in: rect);
      return
    }
    placeholder?.draw(
      in: rect,
      withAttributes: [
        NSAttributedString.Key.font: font,
        NSAttributedString.Key.foregroundColor: tint
    ])
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    for view in subviews {
      if (view is UIButton) {
        let button = view as? UIButton
        button?.setImage(UIImage(named: "ic_search_clear_14"), for: .normal)
        button?.tintColor = tintColor
      }
    }
  }
}
