enum PlacePageStyleSheet: String, CaseIterable {
  case ppTitlePopularView = "PPTitlePopularView"
  case ppActionBarTitle = "PPActionBarTitle"
  case ppActionBarTitlePartner = "PPActionBarTitlePartner"
  case ppElevationProfileDescriptionCell = "ElevationProfileDescriptionCell"
  case ppElevationProfileExtendedDifficulty = "ElevationProfileExtendedDifficulty"
  case ppRouteBasePreview = "RouteBasePreview"
  case ppRoutePreview = "RoutePreview"
  case ppRatingSummaryView24 = "RatingSummaryView24"
  case ppRatingSummaryView12 = "RatingSummaryView12"
  case ppRatingSummaryView12User = "RatingSummaryView12User"
  case ppHeaderView = "PPHeaderView"
  case ppNavigationShadowView = "PPNavigationShadowView"
  case ppBackgroundView = "PPBackgroundView"
  case ppView = "PPView"
  case ppHeaderCircleIcon = "PPHeaderCircleIcon"
  case ppChartView = "ChartView"
  case ppRatingView = "PPRatingView"
  case ppRatingHorrible = "PPRatingHorrible"
  case ppRatingBad = "PPRatingBad"
  case ppRatingNormal = "PPRatingNormal"
  case ppRatingGood = "PPRatingGood"
  case ppRatingExcellent = "PPRatingExellent"
  case ppButton = "PPButton"
}

extension PlacePageStyleSheet: IStyleSheet {
  func styleResolverFor(colors: IColors, fonts: IFonts) -> Theme.StyleResolver {
    switch self {
    case .ppTitlePopularView:
      return .add { s in
        s.backgroundColor = colors.linkBlueHighlighted
        s.cornerRadius = .custom(10)
      }
    case .ppActionBarTitle:
      return .add { s in
        s.font = fonts.regular10
        s.fontColor = colors.blackSecondaryText
      }
    case .ppActionBarTitlePartner:
      return .add { s in
        s.font = fonts.regular10
        s.fontColor = UIColor.white
      }
    case .ppElevationProfileDescriptionCell:
      return .add { s in
        s.backgroundColor = colors.blackOpaque
        s.cornerRadius = .buttonDefault
      }
    case .ppElevationProfileExtendedDifficulty:
      return .add { s in
        s.backgroundColor = colors.blackSecondaryText
        s.fontColor = colors.white
        s.font = fonts.medium14
        s.textContainerInset = UIEdgeInsets(top: 4, left: 6, bottom: 4, right: 6)
      }
    case .ppRouteBasePreview:
      return .add { s in
        s.borderColor = colors.blackDividers
        s.borderWidth = 1
        s.backgroundColor = colors.white
      }
    case .ppRoutePreview:
      return .add { s in
        s.shadowRadius = 2
        s.shadowColor = colors.blackDividers
        s.shadowOpacity = 1
        s.shadowOffset = CGSize(width: 3, height: 0)
        s.backgroundColor = colors.pressBackground
      }
    case .ppRatingSummaryView24:
      return .add { s in
        s.font = fonts.bold16
        s.fontColorHighlighted = colors.ratingYellow
        s.fontColorDisabled = colors.blackDividers
        s.colors = [
          colors.blackSecondaryText,
          colors.ratingRed,
          colors.ratingOrange,
          colors.ratingYellow,
          colors.ratingLightGreen,
          colors.ratingGreen
        ]
        s.images = [
          "ic_24px_rating_normal",
          "ic_24px_rating_horrible",
          "ic_24px_rating_bad",
          "ic_24px_rating_normal",
          "ic_24px_rating_good",
          "ic_24px_rating_excellent"
        ]
      }
    case .ppRatingSummaryView12:
      return .addFrom(Self.ppRatingSummaryView24) { s in
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
    case .ppRatingSummaryView12User:
      return .addFrom(Self.ppRatingSummaryView12) { s in
        s.colors?[0] = colors.linkBlue
        s.images?[0] = "ic_12px_radio_on"
      }
    case .ppHeaderView:
      return .add { s in
        s.backgroundColor = colors.white
        s.cornerRadius = .modalSheet
        s.clip = true
      }
    case .ppNavigationShadowView:
      return .add { s in
        s.backgroundColor = colors.white
        s.shadowColor = UIColor.black
        s.shadowOffset = CGSize(width: 0, height: 1)
        s.shadowOpacity = 0.4
        s.shadowRadius = 1
        s.clip = false
      }
    case .ppBackgroundView:
      return .addFrom(GlobalStyleSheet.modalSheetBackground) { s in
        s.backgroundColor = colors.pressBackground
        s.maskedCorners = isiPad ? CACornerMask.all : [.layerMinXMinYCorner, .layerMaxXMinYCorner]
        s.clip = false
      }
    case .ppView:
      return .add { s in
        s.backgroundColor = colors.clear
        s.cornerRadius = .modalSheet
        s.clip = true
      }
    case .ppHeaderCircleIcon:
      return .add { s in
        s.tintColor = colors.iconOpaqueGrayTint
        s.backgroundColor = colors.iconOpaqueGrayBackground
      }
    case .ppChartView:
      return .add { s in
        s.backgroundColor = colors.white
        s.fontColor = colors.blackSecondaryText
        s.font = fonts.regular12
        s.gridColor = colors.blackDividers
        s.previewSelectorColor = colors.elevationPreviewSelector
        s.previewTintColor = colors.elevationPreviewTint
        s.shadowOpacity = 0.25
        s.shadowColor = colors.shadow
        s.infoBackground = colors.pressBackground
      }
    case .ppRatingView:
      return .add { s in
        s.backgroundColor = colors.blackOpaque
        s.round = true
      }
    case .ppRatingHorrible:
      return .add { s in
        s.image = "ic_24px_rating_horrible"
        s.tintColor = colors.ratingRed
      }
    case .ppRatingBad:
      return .add { s in
        s.image = "ic_24px_rating_bad"
        s.tintColor = colors.ratingOrange
      }
    case .ppRatingNormal:
      return .add { s in
        s.image = "ic_24px_rating_normal"
        s.tintColor = colors.ratingYellow
      }
    case .ppRatingGood:
      return .add { s in
        s.image = "ic_24px_rating_good"
        s.tintColor = colors.ratingLightGreen
      }
    case .ppRatingExcellent:
      return .add { s in
        s.image = "ic_24px_rating_excellent"
        s.tintColor = colors.ratingGreen
      }
    case .ppButton:
      return .addFrom(GlobalStyleSheet.flatNormalTransButtonBig) { s in
        s.borderColor = colors.linkBlue
        s.borderWidth = 1
      }
    }
  }
}
