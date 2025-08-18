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

extension UIColor {
  func components() -> (red: CGFloat, green: CGFloat, blue: CGFloat, alpha: CGFloat)? {
    var r: CGFloat = 0, g: CGFloat = 0, b: CGFloat = 0, a: CGFloat = 0
    return getRed(&r, green: &g, blue: &b, alpha: &a) ? (r,g,b,a) : nil
  }

  static func intermediateColor( color1: UIColor,  color2: UIColor, _ scale: CGFloat) -> UIColor? {
    guard let comp1 = color1.components(),
          let comp2 = color2.components() else {
            return nil
    }
    let scale = min(1, max(0, scale))
    let r = comp1.red + (comp2.red - comp1.red) * scale
    let g = comp1.green + (comp2.green - comp1.green) * scale
    let b = comp1.blue + (comp2.blue - comp1.blue) * scale
    let a = comp1.alpha + (comp2.alpha - comp1.alpha) * scale
    return UIColor(red: r, green: g, blue: b, alpha: a)
  }
}
