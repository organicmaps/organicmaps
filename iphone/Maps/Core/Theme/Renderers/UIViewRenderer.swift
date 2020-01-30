extension UIView {
  @objc func applyTheme() {
    if type(of: self.superview) == UINavigationBar.self {
      return;
    }
    for style in StyleManager.shared.getStyle(styleName)
      where !style.isEmpty && !style.hasExclusion(view: self) {
      UIViewRenderer.render(self, style: style)
    }
  }
}

class UIViewRenderer {
  class func render(_ control: UIView, style: Style) {
    if let backgroundColor = style.backgroundColor {
      control.backgroundColor = backgroundColor
    }
    if let tintColor = style.tintColor {
      control.tintColor = tintColor
    }

    UIViewRenderer.renderShadow(control, style: style)
    UIViewRenderer.renderBorder(control, style: style)
  }

  class func renderShadow(_ control: UIView, style: Style) {
    if let shadowColor = style.shadowColor {
      control.layer.shadowColor = shadowColor.cgColor
    }
    if let shadowOffset = style.shadowOffset {
      control.layer.shadowOffset = shadowOffset
    }
    if let shadowOpacity = style.shadowOpacity {
      control.layer.shadowOpacity = shadowOpacity
    }
    if let shadowRadius = style.shadowRadius {
      control.layer.shadowRadius = shadowRadius
    }
  }

  class func renderBorder(_ control: UIView, style: Style) {
    if let borderColor = style.borderColor {
      control.layer.borderColor = borderColor.cgColor
    }
    if let borderWidth = style.borderWidth {
      control.layer.borderWidth = borderWidth
    }
    if let cornerRadius = style.cornerRadius {
      control.layer.cornerRadius = cornerRadius
    }
    if let clip = style.clip {
      control.clipsToBounds = clip
    }
    if let round = style.round, round == true {
      control.layer.cornerRadius = control.size.height / 2
    }
  }
}
