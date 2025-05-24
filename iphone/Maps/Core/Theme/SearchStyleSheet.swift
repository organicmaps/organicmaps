enum SearchStyleSheet: String, CaseIterable {
  case searchCancelButton
  case searchPopularView = "SearchPopularView"
  case searchSideAvailableMarker = "SearchSideAvaliableMarker"
}

extension SearchStyleSheet: IStyleSheet {
  func styleResolverFor(colors: IColors, fonts: IFonts) -> Theme.StyleResolver {
    switch self {
    case .searchPopularView:
      return .add { s in
        s.cornerRadius = .custom(10)
        s.backgroundColor = colors.linkBlueHighlighted
      }
    case .searchSideAvailableMarker:
      return .add { s in
        s.backgroundColor = colors.ratingGreen
      }
    case .searchCancelButton:
      return .add { s in
        s.fontColor = colors.linkBlue
        s.fontColorHighlighted = colors.linkBlueHighlighted
        s.font = fonts.regular17
        s.backgroundColor = .clear
      }
    }
  }
}
