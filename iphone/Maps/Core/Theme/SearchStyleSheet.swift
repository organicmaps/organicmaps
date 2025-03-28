enum SearchStyleSheet: String, CaseIterable {
  case searchHeader
  case searchCancelButton
  case searchInstallButton = "SearchInstallButton"
  case searchBanner = "SearchBanner"
  case searchClosedBackground = "SearchClosedBackground"
  case searchPopularView = "SearchPopularView"
  case searchSideAvailableMarker = "SearchSideAvaliableMarker"
  case searchBarView = "SearchBarView"
  case searchActionBarView = "SearchActionBarView"
  case searchActionBarButton = "SearchActionBarButton"
  case searchSearchTextField = "SearchSearchTextField"
  case searchSearchTextFieldIcon = "SearchSearchTextFieldIcon"
  case searchDatePickerField = "SearchDatePickerField"
  case searchCellAvailable = "SearchCellAvaliable"
}

extension SearchStyleSheet: IStyleSheet {
  func styleResolverFor(colors: IColors, fonts: IFonts) -> Theme.StyleResolver {
    switch self {
    case .searchHeader:
      return .add { s in
        s.backgroundColor = colors.primary
        iPhoneSpecific {
          s.shadowColor = UIColor.black
          s.shadowOffset = CGSize(width: 0, height: 1)
          s.shadowOpacity = 0.5
          s.shadowRadius = 3
          s.cornerRadius = 10
        }
      }
    case .searchInstallButton:
      return .add { s in
        s.cornerRadius = 10
        s.clip = true
        s.font = fonts.medium12
        s.fontColor = colors.blackSecondaryText
        s.backgroundColor = colors.searchPromoBackground
      }
    case .searchBanner:
      return .add { s in
        s.backgroundColor = colors.searchPromoBackground
      }
    case .searchClosedBackground:
      return .add { s in
        s.cornerRadius = 4
        s.backgroundColor = colors.blackHintText
      }
    case .searchPopularView:
      return .add { s in
        s.cornerRadius = 10
        s.backgroundColor = colors.linkBlueHighlighted
      }
    case .searchSideAvailableMarker:
      return .add { s in
        s.backgroundColor = colors.ratingGreen
      }
    case .searchBarView:
      return .add { s in
        s.backgroundColor = colors.primary
        s.shadowRadius = 2
        s.shadowColor = UIColor(0, 0, 0, alpha26)
        s.shadowOpacity = 1
        s.shadowOffset = CGSize.zero
      }
    case .searchActionBarView:
      return .add { s in
        s.backgroundColor = colors.linkBlue
        s.cornerRadius = 20
        s.shadowRadius = 1
        s.shadowColor = UIColor(0, 0, 0, 0.24)
        s.shadowOffset = CGSize(width: 0, height: 2)
        s.shadowOpacity = 1
      }
    case .searchActionBarButton:
      return .add { s in
        s.backgroundColor = colors.clear
        s.fontColor = colors.whitePrimaryText
        s.font = fonts.semibold14
        s.coloring = .whiteText
      }
    case .searchSearchTextField:
      return .add { s in
        s.fontColor = colors.blackPrimaryText
        s.backgroundColor = colors.white
        s.tintColor = colors.blackSecondaryText
        s.cornerRadius = 8.0
        s.barTintColor = colors.primary
      }
    case .searchSearchTextFieldIcon:
      return .add { s in
        s.tintColor = colors.blackSecondaryText
        s.coloring = MWMButtonColoring.black
        s.color = colors.blackSecondaryText
      }
    case .searchDatePickerField:
      return .add { s in
        s.backgroundColor = colors.white
        s.cornerRadius = 4
        s.borderColor = colors.solidDividers
        s.borderWidth = 1
      }
    case .searchCellAvailable:
      return .addFrom(GlobalStyleSheet.tableCell) { s in
        s.backgroundColor = colors.transparentGreen
      }
    case .searchCancelButton:
      return .add { s in
        s.fontColor = colors.whitePrimaryText
        s.fontColorHighlighted = colors.whitePrimaryTextHighlighted
        s.font = fonts.regular17
        s.backgroundColor = .clear
      }
    }
  }
}
