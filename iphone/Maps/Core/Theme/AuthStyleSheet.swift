class AuthStyleSheet: IStyleSheet {
  static func register(theme: Theme, colors: IColors, fonts: IFonts) {
    theme.add(styleName: "GoogleButton") { (s) -> (Void) in
      s.cornerRadius = 8
      s.borderWidth = 1
      s.borderColor = colors.blackDividers
      s.clip = true
      s.fontColor = colors.blackPrimaryText
      s.fontColorDisabled = colors.blackSecondaryText
      s.font = fonts.bold14
    }

    theme.add(styleName: "FacebookButton") { (s) -> (Void) in
      s.cornerRadius = 8
      s.clip = true
      s.font = fonts.bold14
      s.backgroundColor = colors.facebookButtonBackground
      s.backgroundColorDisabled = colors.facebookButtonBackgroundDisabled
    }

    theme.add(styleName: "OsmSocialLoginButton") { (s) -> (Void) in
      s.font = fonts.regular17
      s.cornerRadius = 8
      s.borderWidth = 1
      s.borderColor = colors.blackDividers
    }
  }
}
