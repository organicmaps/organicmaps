protocol IStyleSheet: CaseIterable, RawRepresentable, StyleStringRepresentable {
  static func register(theme: Theme)
}

extension IStyleSheet {
  static func register(theme: Theme) {
    allCases.forEach { theme.add($0, $0.styleResolver) }
  }
}
