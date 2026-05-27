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
  func styleResolverFor(fonts: IFonts) -> Theme.StyleResolver {
    switch self {
    case .mapMenuButtonDisabled:
      return .add { s in
        s.fontColor = .blackSecondaryText
        s.font = fonts.regular10
        s.backgroundColor = .clear
        s.borderColor = .clear
        s.borderWidth = 0
        s.cornerRadius = .buttonDefault
      }
    case .mapMenuButtonEnabled:
      return .add { s in
        s.fontColor = .linkBlue
        s.font = fonts.regular10
        s.backgroundColor = .linkBlue
        s.borderColor = .linkBlue
        s.borderWidth = 3
        s.cornerRadius = .buttonDefault
      }
    case .mapStreetNameBackgroundView:
      return .add { s in
        s.backgroundColor = .whitePrimary
        s.shadowRadius = 2
        s.shadowColor = .shadow
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
        s.backgroundColor = .linkBlue
        s.cornerRadius = .buttonSmall
        s.shadowRadius = 2
        s.shadowColor = .blackHintText
        s.shadowOpacity = 1
        s.shadowOffset = CGSize(width: 0, height: 2)
      }
    case .mapSecondTurnView:
      return .addFrom(Self.mapFirstTurnView) { s in
        s.backgroundColor = .whitePrimary
      }
    case .mapAutoupdateView:
      return .add { s in
        s.shadowOffset = CGSize(width: 0, height: 3)
        s.shadowRadius = 6
        s.cornerRadius = .buttonSmall
        s.shadowOpacity = 1
        s.backgroundColor = .whitePrimary
      }
    case .mapGuidesNavigationBar:
      return .add { s in
        s.barTintColor = .whitePrimary
        s.tintColor = .linkBlue
        s.backgroundImage = UIImage()
        s.shadowImage = UIImage()
        s.font = fonts.regular18
        s.fontColor = .blackPrimaryText
      }
    }
  }
}
