class MapStyleSheet: IStyleSheet {
  static func register(theme: Theme, colors: IColors, fonts: IFonts) {
    theme.add(styleName: "LayersTrafficButtonEnabled") { (s) -> (Void) in
      s.fontColor = colors.linkBlue
      s.mwmImage = "btn_menu_traffic_on"
    }
    
    theme.add(styleName: "LayersTrafficButtonDisabled") { (s) -> (Void) in
      s.fontColor = colors.blackSecondaryText
      s.mwmImage =  "btn_menu_traffic_off"
    }

    theme.add(styleName: "LayersIsolinesButtonEnabled") { (s) -> (Void) in
      s.fontColor = colors.linkBlue
      s.mwmImage = "btn_menu_isomaps_on"
    }

    theme.add(styleName: "LayersIsolinesButtonDisabled") { (s) -> (Void) in
      s.fontColor = colors.blackSecondaryText
      s.mwmImage = "btn_menu_isomaps_off"
    }

    theme.add(styleName: "LayersSubwayButtonEnabled") { (s) -> (Void) in
      s.fontColor = colors.linkBlue
      s.mwmImage = "btn_menu_subway_on"
    }

    theme.add(styleName: "LayersSubwayButtonDisabled") { (s) -> (Void) in
      s.fontColor = colors.blackSecondaryText
      s.mwmImage = "btn_menu_subway_off"
    }

    theme.add(styleName: "StreetNameBackgroundView") { (s) -> (Void) in
      s.backgroundColor = colors.white
      s.shadowRadius = 2
      s.shadowColor = UIColor(0, 0, 0, alpha26)
      s.shadowOpacity = 1
      s.shadowOffset = CGSize(width: 0, height: 1)
    }

    theme.add(styleName: "PPRatingView") { (s) -> (Void) in
      s.backgroundColor = colors.blackOpaque
      s.round = true
    }
    
    theme.add(styleName: "PPRatingHorrible") { (s) -> (Void) in
      s.image = "ic_24px_rating_horrible"
      s.tintColor = colors.ratingRed
    }

    theme.add(styleName: "PPRatingBad") { (s) -> (Void) in
      s.image = "ic_24px_rating_bad"
      s.tintColor = colors.ratingOrange
    }

    theme.add(styleName: "PPRatingNormal") { (s) -> (Void) in
      s.image = "ic_24px_rating_normal"
      s.tintColor = colors.ratingYellow
    }

    theme.add(styleName: "PPRatingGood") { (s) -> (Void) in
      s.image = "ic_24px_rating_good"
      s.tintColor = colors.ratingLightGreen
    }

    theme.add(styleName: "PPRatingExellent") { (s) -> (Void) in
      s.image = "ic_24px_rating_excellent"
      s.tintColor = colors.ratingGreen
    }

    theme.add(styleName: "PPButton", from: "FlatNormalTransButtonBig") { (s) -> (Void) in
      s.borderColor = colors.linkBlue
      s.borderWidth = 1
    }
    
    theme.add(styleName: "ButtonZoomIn") { (s) -> (Void) in
      s.mwmImage = "btn_zoom_in"
    }

    theme.add(styleName: "ButtonZoomOut") { (s) -> (Void) in
      s.mwmImage = "btn_zoom_out"
    }

    theme.add(styleName: "ButtonPending") { (s) -> (Void) in
      s.mwmImage = "btn_pending"
    }

    theme.add(styleName: "ButtonGetPosition") { (s) -> (Void) in
      s.mwmImage = "btn_get_position"
    }

    theme.add(styleName: "ButtonFollow") { (s) -> (Void) in
      s.mwmImage = "btn_follow"
    }

    theme.add(styleName: "ButtonFollowAndRotate") { (s) -> (Void) in
      s.mwmImage = "btn_follow_and_rotate"
    }

    theme.add(styleName: "ButtonMapBookmarks") { (s) -> (Void) in
      s.mwmImage = "ic_routing_bookmark"
    }

    theme.add(styleName: "PromoDiscroveryButton") { (s) -> (Void) in
      s.mwmImage = "promo_discovery_button"
    }

    theme.add(styleName: "FirstTurnView") { (s) -> (Void) in
      s.backgroundColor = colors.linkBlue
      s.cornerRadius = 4
      s.shadowRadius = 2
      s.shadowColor = colors.blackHintText
      s.shadowOpacity = 1
      s.shadowOffset = CGSize(width: 0, height: 2)
    }

    theme.add(styleName: "SecondTurnView", from: "FirstTurnView") { (s) -> (Void) in
      s.backgroundColor = colors.white
    }

    theme.add(styleName: "MapAutoupdateView") { (s) -> (Void) in
      s.shadowOffset = CGSize(width: 0, height: 3)
      s.shadowRadius = 6
      s.cornerRadius = 4
      s.shadowOpacity = 1
      s.backgroundColor = colors.white
    }

    theme.add(styleName: "PPReviewDiscountView") { (s) -> (Void) in
      s.backgroundColor = colors.linkBlue
      s.round = true
    }

    theme.add(styleName: "PPTitlePopularView") { (s) -> (Void) in
      s.backgroundColor = colors.linkBlueHighlighted
      s.cornerRadius = 10
    }

    theme.add(styleName: "RouteBasePreview") { (s) -> (Void) in
      s.borderColor = colors.blackDividers
      s.borderWidth = 1
      s.backgroundColor = colors.white
    }

    theme.add(styleName: "RoutePreview") { (s) -> (Void) in
      s.shadowRadius = 2
      s.shadowColor = colors.blackDividers
      s.shadowOpacity = 1
      s.shadowOffset = CGSize(width: 3, height: 0)
      s.backgroundColor = colors.pressBackground
    }

    theme.add(styleName: "RatingSummaryView24") { (s) -> (Void) in
      s.font = fonts.bold16
      s.fontColorHighlighted = colors.ratingYellow //filled color
      s.fontColorDisabled = colors.blackDividers //empty color
      s.colors = [
        colors.blackSecondaryText, //noValue
        colors.ratingRed, //horrible
        colors.ratingOrange, //bad
        colors.ratingYellow, //normal
        colors.ratingLightGreen, //good
        colors.ratingGreen //exellent
      ]
      s.images = [
        "ic_24px_rating_normal", //noValue
        "ic_24px_rating_horrible", //horrible
        "ic_24px_rating_bad", //bad
        "ic_24px_rating_normal", //normal
        "ic_24px_rating_good", //good
        "ic_24px_rating_excellent" //exellent
      ]
    }

    theme.add(styleName: "RatingSummaryView12", from: "RatingSummaryView24") { (s) -> (Void) in
      s.font = fonts.bold12
      s.images = [
        "ic_12px_rating_normal",
        "ic_12px_rating_horrible",
        "ic_12px_rating_bad",
        "ic_12px_rating_normal",
        "ic_12px_rating_good",
        "ic_12px_rating_excellent"
      ]
    }

    theme.add(styleName: "RatingSummaryView12User", from: "RatingSummaryView12") { (s) -> (Void) in
      s.colors?[0] = colors.linkBlue
      s.images?[0] = "ic_12px_radio_on"
    }
  }
}
