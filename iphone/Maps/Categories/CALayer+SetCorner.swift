extension CALayer {
  func setCornerRadius(_ cornerRadius: CornerRadius,
                       maskedCorners: CACornerMask? = nil) {
    self.cornerRadius = cornerRadius.value
    if let maskedCorners {
      self.maskedCorners = maskedCorners
    }
    if #available(iOS 13.0, *) {
      cornerCurve = .continuous
    }
  }
}

extension CACornerMask {
  static var all: CACornerMask {
    return [.layerMinXMinYCorner, .layerMaxXMinYCorner, .layerMinXMaxYCorner, .layerMaxXMaxYCorner]
  }
}
