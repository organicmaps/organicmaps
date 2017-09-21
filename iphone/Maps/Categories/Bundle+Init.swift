extension Bundle {
  @objc func load(viewClass: AnyClass, owner: Any? = nil, options: [AnyHashable: Any]? = nil) -> [Any]? {
    return loadNibNamed(toString(viewClass), owner: owner, options: options)
  }
}
