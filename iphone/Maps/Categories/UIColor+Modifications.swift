extension UIColor {
  func blending(with color: UIColor) -> UIColor {
    var bgR: CGFloat = 0
    var bgG: CGFloat = 0
    var bgB: CGFloat = 0
    var bgA: CGFloat = 0
    
    var fgR: CGFloat = 0
    var fgG: CGFloat = 0
    var fgB: CGFloat = 0
    var fgA: CGFloat = 0
    
    self.getRed(&bgR, green: &bgG, blue: &bgB, alpha: &bgA)
    color.getRed(&fgR, green: &fgG, blue: &fgB, alpha: &fgA)
    
    let r = fgA * fgR + (1 - fgA) * bgR
    let g = fgA * fgG + (1 - fgA) * bgG
    let b = fgA * fgB + (1 - fgA) * bgB
    
    return UIColor(red: r, green: g, blue: b, alpha: bgA)
  }

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
