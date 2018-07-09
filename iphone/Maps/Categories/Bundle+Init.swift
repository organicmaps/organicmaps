extension Bundle {
  @objc func load(viewClass: AnyClass, owner: Any? = nil, options: [AnyHashable: Any]? = nil) -> [Any]? {
    return loadNibNamed(toString(viewClass), owner: owner, options: options)
  }

  @objc func load(plist: String) -> Dictionary<String, AnyObject>? {
    guard let path = Bundle.main.path(forResource: plist, ofType: "plist") else { return nil }
    return NSDictionary(contentsOfFile: path) as? [String: AnyObject]
  }
}
