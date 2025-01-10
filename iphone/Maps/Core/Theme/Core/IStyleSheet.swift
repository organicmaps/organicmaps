protocol IStyleSheet: CaseIterable, RawRepresentable, StyleStringRepresentable {
  static func register(theme: Theme, colors: IColors, fonts: IFonts)
}

extension IStyleSheet {
  static func register(theme: Theme, colors: IColors, fonts: IFonts) {
    allCases.forEach { theme.add($0, $0.styleResolverFor(colors: colors, fonts: fonts)) }
  }
}
