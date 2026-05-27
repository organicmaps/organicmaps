enum SearchStyleSheet: String, CaseIterable {
  case searchOnMapSearchBar
  case searchCancelButton
  case searchPopularView = "SearchPopularView"
  case searchSideAvailableMarker = "SearchSideAvaliableMarker"
}

extension SearchStyleSheet: IStyleSheet {
  func styleResolverFor(fonts: IFonts) -> Theme.StyleResolver {
    switch self {
    case .searchOnMapSearchBar:
      return .add { s in
        if #available(iOS 26.0, *) {
          s.backgroundColor = .lightGray.withAlphaComponent(0.2)
        } else {
          s.backgroundColor = .pressBackground
          s.barTintColor = .clear
          s.fontColor = .blackPrimaryText
          s.fontColorDetailed = UIColor.whitePrimary
          s.tintColor = .blackSecondaryText
        }
      }
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
