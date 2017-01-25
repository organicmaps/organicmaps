extension UINib {
  convenience init(_ viewClass: AnyClass, bundle: Bundle? = nil) {
    self.init(nibName: toString(viewClass), bundle: bundle)
  }
}
