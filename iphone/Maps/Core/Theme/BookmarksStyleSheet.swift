enum BookmarksStyleSheet: String, CaseIterable {
  case bookmarksCategoryTextView = "BookmarksCategoryTextView"
  case bookmarksCategoryDeleteButton = "BookmarksCategoryDeleteButton"
  case bookmarksActionCreateIcon = "BookmarksActionCreateIcon"
  case bookmarkSharingLicense = "BookmarkSharingLicense"
}

extension BookmarksStyleSheet: IStyleSheet {
  func styleResolverFor(colors: IColors, fonts: IFonts) -> Theme.StyleResolver {
    switch self {
    case .bookmarksCategoryTextView:
      return .add { s in
        s.font = fonts.regular16
        s.fontColor = colors.blackPrimaryText
        s.textContainerInset = UIEdgeInsets(top: 0, left: 0, bottom: 0, right: 0)
      }
    case .bookmarksCategoryDeleteButton:
      return .add { s in
        s.font = fonts.regular17
        s.fontColor = colors.red
        s.fontColorDisabled = colors.blackHintText
      }
    case .bookmarksActionCreateIcon:
      return .add { s in
        s.tintColor = colors.linkBlue
      }
    case .bookmarkSharingLicense:
      return .addFrom(GlobalStyleSheet.termsOfUseLinkText) { s in
        s.fontColor = colors.blackSecondaryText
        s.font = fonts.regular14
      }
    }
  }
}
