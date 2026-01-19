extension Bundle {
  func load<T: UIView>(viewClass: T.Type, owner: Any? = nil, options: [AnyHashable: Any]? = nil) -> T? {
    loadNibNamed(String(describing: viewClass), owner: owner, options: options as? [UINib.OptionsKey: Any])?.first as? T
  }

  @objc func load(viewClass: AnyClass, owner: Any? = nil, options: [AnyHashable: Any]? = nil) -> [Any]? {
    loadNibNamed(toString(viewClass), owner: owner, options: options as? [UINib.OptionsKey: Any])
  }

  @objc func load(plist: String) -> [String: AnyObject]? {
    guard let path = Bundle.main.path(forResource: plist, ofType: "plist") else { return nil }
    return NSDictionary(contentsOfFile: path) as? [String: AnyObject]
  }
}
