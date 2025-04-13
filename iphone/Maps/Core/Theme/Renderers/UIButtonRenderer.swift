extension UIButton {
  @objc override func applyTheme() {
    for style in StyleManager.shared.getStyle(styleName)
      where !style.isEmpty && !style.hasExclusion(view: self) {
      UIButtonRenderer.render(self, style: style)
    }
  }
}

class UIButtonRenderer {
  class func render(_ control: UIButton, style: Style) {
    if let titleLabel = control.titleLabel {
      if let font = style.font {
        titleLabel.font = font
      }
    }

    if let fontColor = style.fontColor {
      control.setTitleColor(fontColor, for: .normal)
    }

    if let backgroundColor = style.backgroundColor {
      control.setBackgroundImage(backgroundColor.getImage(), for: .normal)
      control.backgroundColor = UIColor.clear
    }
    if let backgroundColorSelected = style.backgroundColorSelected {
      control.setBackgroundImage(backgroundColorSelected.getImage(), for: .selected)
    }
    if let backgroundColorHighlighted = style.backgroundColorHighlighted {
      control.setBackgroundImage(backgroundColorHighlighted.getImage(), for: .highlighted)
    }
    if let backgroundColorDisabled = style.backgroundColorDisabled {
      control.setBackgroundImage(backgroundColorDisabled.getImage(), for: .disabled)
    }
    if let fontColorSelected = style.fontColorSelected {
      control.setTitleColor(fontColorSelected, for: .selected)
    }
    if let fontColorHighlighted = style.fontColorHighlighted {
      control.setTitleColor(fontColorHighlighted, for: .highlighted)
    }
    if let fontColorDisabled = style.fontColorDisabled {
      control.setTitleColor(fontColorDisabled, for: .disabled)
    }
    if let image = style.image {
      control.setImage(UIImage(named: image), for: .normal)
    }
    if let tintColor = style.tintColor {
      control.tintColor = tintColor
    }

    UIViewRenderer.renderBorder(control, style: style)
    UIViewRenderer.renderShadow(control, style: style)
  }
}

