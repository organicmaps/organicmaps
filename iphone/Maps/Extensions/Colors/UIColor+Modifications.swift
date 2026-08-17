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

    getRed(&bgR, green: &bgG, blue: &bgB, alpha: &bgA)
    color.getRed(&fgR, green: &fgG, blue: &fgB, alpha: &fgA)

    let r = fgA * fgR + (1 - fgA) * bgR
    let g = fgA * fgG + (1 - fgA) * bgG
    let b = fgA * fgB + (1 - fgA) * bgB

    return UIColor(red: r, green: g, blue: b, alpha: bgA)
  }

  func interpolated(to color: UIColor, progress: CGFloat) -> UIColor {
    let progress = max(0, min(1, progress))
    var fromRed: CGFloat = 0
    var fromGreen: CGFloat = 0
    var fromBlue: CGFloat = 0
    var fromAlpha: CGFloat = 0
    var toRed: CGFloat = 0
    var toGreen: CGFloat = 0
    var toBlue: CGFloat = 0
    var toAlpha: CGFloat = 0

    guard getRed(&fromRed, green: &fromGreen, blue: &fromBlue, alpha: &fromAlpha),
          color.getRed(&toRed, green: &toGreen, blue: &toBlue, alpha: &toAlpha) else {
      return progress < 0.5 ? self : color
    }

    return UIColor(red: fromRed + (toRed - fromRed) * progress,
                   green: fromGreen + (toGreen - fromGreen) * progress,
                   blue: fromBlue + (toBlue - fromBlue) * progress,
                   alpha: fromAlpha + (toAlpha - fromAlpha) * progress)
  }

  func lighter(percent: CGFloat) -> UIColor {
    colorWithBrightnessFactor(factor: 1 + percent)
  }

  func darker(percent: CGFloat) -> UIColor {
    colorWithBrightnessFactor(factor: 1 - percent)
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
