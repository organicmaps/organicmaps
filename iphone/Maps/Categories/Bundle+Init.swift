extension Bundle {
  func load<T:UIView>(viewClass: T.Type, owner: Any? = nil, options: [AnyHashable: Any]? = nil) -> T? {
    return loadNibNamed(String(describing: viewClass), owner: owner, options: options as? [UINib.OptionsKey : Any])?.first as? T
  }

  @objc func load(viewClass: AnyClass, owner: Any? = nil, options: [AnyHashable: Any]? = nil) -> [Any]? {
    return loadNibNamed(toString(viewClass), owner: owner, options: options as? [UINib.OptionsKey : Any])
  }

  @objc func load(plist: String) -> Dictionary<String, AnyObject>? {
    guard let path = Bundle.main.path(forResource: plist, ofType: "plist") else { return nil }
    return NSDictionary(contentsOfFile: path) as? [String: AnyObject]
  }
}
