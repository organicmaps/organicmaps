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
  func styleResolverFor(fonts: IFonts) -> Theme.StyleResolver {
    switch self {
    case .ppTitlePopularView:
      return .add { s in
        s.backgroundColor = .linkBlueHighlighted
        s.cornerRadius = .custom(10)
      }
    case .ppActionBarTitle:
      return .add { s in
        s.font = fonts.regular10
        s.fontColor = .blackSecondaryText
      }
    case .ppActionBarTitlePartner:
      return .add { s in
        s.font = fonts.regular10
        s.fontColor = UIColor.whitePrimary
      }
    case .ppElevationProfileDescriptionCell:
      return .add { s in
        s.backgroundColor = .blackOpaque
        s.cornerRadius = .buttonDefault
      }
    case .ppElevationProfileExtendedDifficulty:
      return .add { s in
        s.backgroundColor = .blackSecondaryText
        s.fontColor = .whitePrimary
        s.font = fonts.medium14
        s.textContainerInset = UIEdgeInsets(top: 4, left: 6, bottom: 4, right: 6)
      }
    case .ppRouteBasePreview:
      return .add { s in
        s.borderColor = .blackDividers
        s.borderWidth = 1
        s.backgroundColor = .whitePrimary
      }
    case .ppRoutePreview:
      return .add { s in
        s.shadowRadius = 2
        s.shadowColor = .blackDividers
        s.shadowOpacity = 1
        s.shadowOffset = CGSize(width: 3, height: 0)
        s.backgroundColor = .pressBackground
      }
    case .ppRatingSummaryView24:
      return .add { s in
        s.font = fonts.bold16
        s.fontColorHighlighted = .ratingYellow
        s.fontColorDisabled = .blackDividers
        s.colors = [
          .blackSecondaryText,
          .ratingRed,
          .ratingOrange,
          .ratingYellow,
          .ratingLightGreen,
          .ratingGreen,
        ]
        s.images = [
          "ic_24px_rating_normal",
          "ic_24px_rating_horrible",
          "ic_24px_rating_bad",
          "ic_24px_rating_normal",
          "ic_24px_rating_good",
          "ic_24px_rating_excellent",
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
          "ic_12px_rating_excellent",
        ]
      }
    case .ppRatingSummaryView12User:
      return .addFrom(Self.ppRatingSummaryView12) { s in
        s.colors?[0] = .linkBlue
        s.images?[0] = "ic_12px_radio_on"
      }
    case .ppHeaderView:
      return .add { s in
        s.backgroundColor = .whitePrimary
        s.cornerRadius = .modalSheet
        s.clip = true
      }
    case .ppNavigationShadowView:
      return .add { s in
        s.backgroundColor = .whitePrimary
        s.shadowColor = UIColor.black
        s.shadowOffset = CGSize(width: 0, height: 1)
        s.shadowOpacity = 0.3
        s.shadowRadius = 6
        s.clip = false
      }
    case .ppBackgroundView:
      return .addFrom(GlobalStyleSheet.modalSheetBackground) { s in
        s.backgroundColor = .pressBackground
        s.maskedCorners = .all
      }
    case .ppView:
      return .add { s in
        s.backgroundColor = .clear
        s.maskedCorners = [.layerMinXMinYCorner, .layerMaxXMinYCorner]
        s.cornerRadius = .modalSheet
        s.clip = true
      }
    case .ppHeaderCircleIcon:
      return .add { s in
        s.tintColor = .iconOpaqueGrayTint
        s.backgroundColor = .iconOpaqueGrayBackground
      }
    case .ppChartView:
      return .add { s in
        s.backgroundColor = .whitePrimary
        s.fontColor = .blackSecondaryText
        s.font = fonts.regular12
        s.gridColor = .blackDividers
        s.previewSelectorColor = .elevationPreviewSelector
        s.previewTintColor = .elevationPreviewTint
        s.shadowOpacity = 0.25
        s.shadowColor = .shadow
        s.infoBackground = .pressBackground
      }
    case .ppRatingView:
      return .add { s in
        s.backgroundColor = .blackOpaque
        s.round = true
      }
    case .ppRatingHorrible:
      return .add { s in
        s.image = "ic_24px_rating_horrible"
        s.tintColor = .ratingRed
      }
    case .ppRatingBad:
      return .add { s in
        s.image = "ic_24px_rating_bad"
        s.tintColor = .ratingOrange
      }
    case .ppRatingNormal:
      return .add { s in
        s.image = "ic_24px_rating_normal"
        s.tintColor = .ratingYellow
      }
    case .ppRatingGood:
      return .add { s in
        s.image = "ic_24px_rating_good"
        s.tintColor = .ratingLightGreen
      }
    case .ppRatingExcellent:
      return .add { s in
        s.image = "ic_24px_rating_excellent"
        s.tintColor = .ratingGreen
      }
    case .ppButton:
      return .addFrom(GlobalStyleSheet.flatNormalTransButtonBig) { s in
        s.borderColor = .linkBlue
        s.borderWidth = 1
      }
    }
  }
}
