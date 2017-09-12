extension UIColor {
  func lighter(percent: CGFloat) -> UIColor {
    return colorWithBrightnessFactor(factor: 1 + percent)
  }

  func darker(percent: CGFloat) -> UIColor {
    return colorWithBrightnessFactor(factor: 1 - percent)
  }

  private func colorWithBrightnessFactor(factor: CGFloat) -> UIColor {
    var hue: CGFloat = 0
    var saturation: CGFloat = 0
    var brightness: CGFloat = 0
    var alpha: CGFloat = 0

    if getHue(&hue, saturation: &saturation, brightness: &brightness, alpha: &alpha) {
      return UIColor(hue: hue, saturation: saturation, brightness: brightness * factor, alpha: alpha)
    } else {
      return self
    }
  }
}
