extension CALayer {
  func setCorner(radius: CGFloat,
                 corners: CACornerMask? = nil) {
    cornerRadius = radius
    if let corners {
      maskedCorners = corners
    }
    if #available(iOS 13.0, *) {
      cornerCurve = .continuous
    }
  }
}
