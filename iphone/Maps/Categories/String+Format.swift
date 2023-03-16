extension String {
  init(coreFormat: String, arguments: [CVarArg]) {
    let format = coreFormat.replacingOccurrences(of: "%s", with: "%@")
    self.init(format: format, arguments: arguments.map { "\($0)" })
  }
}

extension NSString {
  @objc
  static func string(coreFormat: String, arguments: [AnyObject]) -> NSString {
    return NSString(coreFormat: coreFormat, arguments: arguments)
  }

  @objc
  convenience init(coreFormat: String, arguments: [AnyObject]) {
    self.init(string: String(coreFormat: coreFormat, arguments: arguments as! [CVarArg]))
  }
}
