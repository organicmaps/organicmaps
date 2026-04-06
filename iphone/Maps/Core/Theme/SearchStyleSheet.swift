enum SearchStyleSheet: String, CaseIterable {
  case searchCancelButton
  case searchPopularView = "SearchPopularView"
  case searchSideAvailableMarker = "SearchSideAvaliableMarker"
}

extension SearchStyleSheet: IStyleSheet {
  func styleResolverFor(fonts: IFonts) -> Theme.StyleResolver {
    switch self {
    case .searchPopularView:
      return .add { s in
        s.cornerRadius = .custom(10)
        s.backgroundColor = .linkBlueHighlighted
      }
    case .searchSideAvailableMarker:
      return .add { s in
        s.backgroundColor = .ratingGreen
      }
    case .searchCancelButton:
      return .add { s in
        s.fontColor = .linkBlue
        s.fontColorHighlighted = .linkBlueHighlighted
        s.font = fonts.regular17
        s.backgroundColor = .clear
      }
    }
  }
}
