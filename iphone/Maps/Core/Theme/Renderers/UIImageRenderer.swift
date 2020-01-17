extension UIImageView {
  @objc override func applyTheme() {
    for style in StyleManager.instance().getStyle(styleName) {
      UIViewRenderer.render(self, style: style)
      UIImageViewRenderer.render(self, style: style)
    }
  }
}

class UIImageViewRenderer {
  class func render(_ control: UIImageView, style: Style) {
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
