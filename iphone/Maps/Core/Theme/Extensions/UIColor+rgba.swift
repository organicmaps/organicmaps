extension UIColor {
  convenience init(_ r: CGFloat, _ g: CGFloat, _ b :CGFloat, _ a: CGFloat) {
    self.init(red: CGFloat(r/255.0), green: CGFloat(g/255.0), blue: CGFloat(b/255.0), alpha: a)
  }
}
