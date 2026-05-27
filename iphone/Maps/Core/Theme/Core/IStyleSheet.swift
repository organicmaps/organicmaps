protocol IStyleSheet: CaseIterable, RawRepresentable, StyleStringRepresentable {
  static func register(theme: Theme, fonts: IFonts)
}

extension IStyleSheet {
  static func register(theme: Theme, fonts: IFonts) {
    allCases.forEach { theme.add($0, $0.styleResolverFor(fonts: fonts)) }
  }
}
