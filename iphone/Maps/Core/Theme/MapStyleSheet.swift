enum MapStyleSheet: String, CaseIterable {
  case mapMenuButtonDisabled = "MenuButtonDisabled"
  case mapMenuButtonEnabled = "MenuButtonEnabled"
  case mapStreetNameBackgroundView = "StreetNameBackgroundView"
  case mapButtonZoomIn = "ButtonZoomIn"
  case mapButtonZoomOut = "ButtonZoomOut"
  case mapButtonPending = "ButtonPending"
  case mapButtonGetPosition = "ButtonGetPosition"
  case mapButtonFollow = "ButtonFollow"
  case mapButtonFollowAndRotate = "ButtonFollowAndRotate"
  case mapButtonMapBookmarks = "ButtonMapBookmarks"
  case mapPromoDiscoveryButton = "PromoDiscroveryButton"
  case mapButtonBookmarksBack = "ButtonBookmarksBack"
  case mapButtonBookmarksBackOpaque = "ButtonBookmarksBackOpaque"
  case mapFirstTurnView = "FirstTurnView"
  case mapSecondTurnView = "SecondTurnView"
  case mapAutoupdateView = "MapAutoupdateView"
  case mapGuidesNavigationBar = "GuidesNavigationBar"
}

extension MapStyleSheet: IStyleSheet {
  func styleResolverFor(colors: IColors, fonts: IFonts) -> Theme.StyleResolver {
    switch self {
    case .mapMenuButtonDisabled:
      return .add { s in
        s.fontColor = colors.blackSecondaryText
        s.font = fonts.regular10
        s.backgroundColor = colors.clear
        s.borderColor = colors.clear
        s.borderWidth = 0
        s.cornerRadius = 6
      }
    case .mapMenuButtonEnabled:
      return .add { s in
        s.fontColor = colors.linkBlue
        s.font = fonts.regular10
        s.backgroundColor = colors.linkBlue
        s.borderColor = colors.linkBlue
        s.borderWidth = 2
        s.cornerRadius = 6
      }
    case .mapStreetNameBackgroundView:
      return .add { s in
        s.backgroundColor = colors.white
        s.shadowRadius = 2
        s.shadowColor = UIColor(0, 0, 0, alpha26)
        s.shadowOpacity = 1
        s.shadowOffset = CGSize(width: 0, height: 1)
      }
    case .mapButtonZoomIn:
      return .add { s in
        s.mwmImage = "btn_zoom_in"
      }
    case .mapButtonZoomOut:
      return .add { s in
        s.mwmImage = "btn_zoom_out"
      }
    case .mapButtonPending:
      return .add { s in
        s.mwmImage = "btn_pending"
      }
    case .mapButtonGetPosition:
      return .add { s in
        s.mwmImage = "btn_get_position"
      }
    case .mapButtonFollow:
      return .add { s in
        s.mwmImage = "btn_follow"
      }
    case .mapButtonFollowAndRotate:
      return .add { s in
        s.mwmImage = "btn_follow_and_rotate"
      }
    case .mapButtonMapBookmarks:
      return .add { s in
        s.mwmImage = "ic_routing_bookmark"
      }
    case .mapPromoDiscoveryButton:
      return .add { s in
        s.mwmImage = "promo_discovery_button"
      }
    case .mapButtonBookmarksBack:
      return .add { s in
        s.mwmImage = "btn_back"
      }
    case .mapButtonBookmarksBackOpaque:
      return .add { s in
        s.mwmImage = "btn_back_opaque"
      }
    case .mapFirstTurnView:
      return .add { s in
        s.backgroundColor = colors.linkBlue
        s.cornerRadius = 4
        s.shadowRadius = 2
        s.shadowColor = colors.blackHintText
        s.shadowOpacity = 1
        s.shadowOffset = CGSize(width: 0, height: 2)
      }
    case .mapSecondTurnView:
      return .addFrom(Self.mapFirstTurnView) { s in
        s.backgroundColor = colors.white
      }
    case .mapAutoupdateView:
      return .add { s in
        s.shadowOffset = CGSize(width: 0, height: 3)
        s.shadowRadius = 6
        s.cornerRadius = 4
        s.shadowOpacity = 1
        s.backgroundColor = colors.white
      }
    case .mapGuidesNavigationBar:
      return .add { s in
        s.barTintColor = colors.white
        s.tintColor = colors.linkBlue
        s.backgroundImage = UIImage()
        s.shadowImage = UIImage()
        s.font = fonts.regular18
        s.fontColor = colors.blackPrimaryText
      }
    }
  }
}
