protocol IStyleSheet: AnyObject {
  static func register(theme: Theme, colors: IColors, fonts: IFonts)
}
