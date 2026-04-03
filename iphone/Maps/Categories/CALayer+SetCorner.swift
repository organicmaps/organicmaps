extension CALayer {
  func setCornerRadius(_ cornerRadius: CornerRadius,
                       maskedCorners: CACornerMask? = nil) {
    self.cornerRadius = cornerRadius.value
    if let maskedCorners {
      self.maskedCorners = maskedCorners
    }
    cornerCurve = .continuous
  }
}

extension CACornerMask {
  static var all: CACornerMask {
    [.layerMinXMinYCorner, .layerMaxXMinYCorner, .layerMinXMaxYCorner, .layerMaxXMaxYCorner]
  }
}
