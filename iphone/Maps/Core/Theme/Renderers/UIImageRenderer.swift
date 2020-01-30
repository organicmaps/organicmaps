extension UIImageView {
  @objc override func applyTheme() {
    for style in StyleManager.shared.getStyle(styleName)
      where !style.isEmpty && !style.hasExclusion(view: self) {
      UIImageViewRenderer.render(self, style: style)
    }
  }
}

class UIImageViewRenderer: UIViewRenderer {
  class func render(_ control: UIImageView, style: Style) {
    super.render(control, style: style)
    if let image = style.image {
      control.image = UIImage(named: image)
    }
    if let tintColor = style.tintColor {
      control.tintColor = tintColor
    }
    if let imageName = style.mwmImage {
      control.mwm_name = imageName
    }
  }
}
