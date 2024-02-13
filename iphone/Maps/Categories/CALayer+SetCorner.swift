extension CALayer {
  func setCorner(radius: CGFloat,
                 curve: CALayerCornerCurve? = nil,
                 corners: CACornerMask? = nil) {
    cornerRadius = radius
    if let corners {
      maskedCorners = corners
    }
    if #available(iOS 13.0, *) {
      cornerCurve = curve ?? .continuous
    }
  }
}
