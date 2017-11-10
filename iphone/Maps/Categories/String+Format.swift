extension String {
  init(coreFormat: String, arguments: [CVarArg]) {
    let format = coreFormat.replacingOccurrences(of: "%s", with: "%@")
    self.init(format: format, arguments: arguments.map { "\($0)" })
  }
}
