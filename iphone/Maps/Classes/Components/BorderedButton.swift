@objc(MWMBorderedButton)
final class BorderedButton: UIButton {

  private var borderColor: UIColor?
  private var borderHighlightedColor: UIColor?
  private var borderDisabledColor: UIColor?

  func setBorderColor(_ color: UIColor) {
    borderColor = color
  }

  func setBorderHighlightedColor(_ color: UIColor) {
    borderHighlightedColor = color
  }

  func setBorderDisabledColor(_ color: UIColor) {
    borderDisabledColor = color
  }

  private func updateBorder() {
    if !isEnabled {
      layer.borderColor = borderDisabledColor?.cgColor ?? titleColor(for: .disabled)?.cgColor
    } else if isHighlighted {
      layer.borderColor = borderHighlightedColor?.cgColor ?? titleColor(for: .highlighted)?.cgColor
    } else {
      layer.borderColor = borderColor?.cgColor ?? titleColor(for: .normal)?.cgColor
    }
  }

  override var isEnabled: Bool {
    didSet { updateBorder() }
  }

  override var isHighlighted: Bool {
    didSet { updateBorder() }
  }
}
