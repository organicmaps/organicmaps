class SearchTextField: UITextField {
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
        let clearButtonImage: UIImage?
        if #available(iOS 13.0, *) {
          clearButtonImage = UIImage(named: "ic_clear")?.withRenderingMode(.alwaysTemplate).withTintColor(tintColor)
        } else {
          clearButtonImage = UIImage(named: "ic_search_clear_14")
        }
        button?.setImage(clearButtonImage, for: .normal)
        button?.tintColor = tintColor
      }
    }
  }
}
