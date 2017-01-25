extension Bundle {
  func load(viewClass: AnyClass, owner: Any?, options: [AnyHashable : Any]? = nil) -> [Any]? {
    return loadNibNamed(toString(viewClass), owner: owner, options: options)
  }
}
