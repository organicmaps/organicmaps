import Foundation

class BookmarksStyleSheet: IStyleSheet {
  static func register(theme: Theme, colors: IColors, fonts: IFonts) {
    theme.add(styleName: "BookmarksCategoryTextView") { (s) -> (Void) in
      s.font = fonts.regular16
      s.fontColor = colors.blackPrimaryText
      s.textContainerInset = UIEdgeInsets(top: 0, left: 0, bottom: 0, right: 0)
    }

    theme.add(styleName: "BookmarksCategoryDeleteButton") { (s) -> (Void) in
      s.font = fonts.regular17
      s.fontColor = colors.red
      s.fontColorDisabled = colors.blackHintText
    }

    theme.add(styleName: "BookmarksActionCreateIcon") { (s) -> (Void) in
      s.tintColor = colors.linkBlue
    }

    theme.add(styleName: "LonelyPlanetLogo") { (s) -> (Void) in
      s.tintColor = colors.lonelyPlanetLogoColor
    }

    theme.add(styleName: "BookmarkSharingLicense", from: "TermsOfUseLinkText") { (s) -> (Void) in
      s.fontColor = colors.blackSecondaryText
      s.font = fonts.regular14
    }
  }
}
