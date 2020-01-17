class AdsStyleSheet: IStyleSheet {
  static func register(theme: Theme, colors: IColors, fonts: IFonts) {
    theme.add(styleName: "NativeAdView") { (s) -> (Void) in
      s.borderWidth = 2
      s.borderColor = UIColor.init(0, 0, 0, alpha12);
      s.backgroundColor = colors.bannerBackground
    }

    theme.add(styleName: "AdBannerPrivacyImage") { (s) -> (Void) in
      s.cornerRadius = 4
      s.clip = true
    }
    
    theme.add(styleName: "AdBannerButton") { (s) -> (Void) in
      s.font = fonts.medium14
      s.backgroundColor = colors.bannerButtonBackground
      s.fontColor = colors.blackSecondaryText
      s.clip = true
      s.cornerRadius = 8
    }

    theme.add(styleName: "AdCallToActionButton", from: "AdBannerButton") { (s) -> (Void) in
      s.cornerRadius = 10
    }

    theme.add(styleName: "RemoveAdsOptionsButton") { (s) -> (Void) in
      s.font = fonts.regular12
      s.fontColor = colors.blackSecondaryText
      s.fontColorHighlighted = colors.blackHintText
      s.backgroundColor = colors.border
      s.backgroundColorSelected = colors.blackDividers
    }

    theme.add(styleName: "RemoveAdsOptionsView") { (s) -> (Void) in
      s.borderColor = colors.blackDividers
      s.borderWidth = 1
      s.backgroundColor = colors.blackDividers
      s.cornerRadius = 9
    }

    theme.add(styleName: "AdsIconImage") { (s) -> (Void) in
      s.cornerRadius = 4
      s.clip = true
      s.borderWidth = 1
      s.borderColor = colors.blackDividers
    }
  }
}
