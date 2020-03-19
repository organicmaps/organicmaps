class SubscriptionsStyleSheet: IStyleSheet {
  static func register(theme: Theme, colors: IColors, fonts: IFonts) {
    theme.add(styleName: "BookmarkSubscriptionScrollBackground") { (s) -> (Void) in
      s.backgroundColor = colors.bookmarkSubscriptionScrollBackground
    }
    
    theme.add(styleName: "BookmarkSubscriptionBackground") { (s) -> (Void) in
      s.backgroundColor = colors.bookmarkSubscriptionBackground
    }

    theme.add(styleName: "BookmarkSubscriptionFooterBackground") { (s) -> (Void) in
      s.backgroundColor = colors.bookmarkSubscriptionFooterBackground
    }

    theme.add(styleName: "BookmarksSubscriptionMonthlyButton") { (s) -> (Void) in
      s.font = fonts.medium14
      s.fontColor = colors.blackHintText
      s.borderWidth = 1
      s.cornerRadius = 6
      s.borderColor = colors.blackHintText
      s.backgroundColorHighlighted = colors.blackDividers
    }

    theme.add(styleName: "BookmarksSubscriptionDiscount", forType: .dark) { (s) -> (Void) in
      s.backgroundColor = colors.discountBackground
      s.cornerRadius = 6
      s.shadowRadius = 4
      s.shadowOffset = CGSize(width: 0, height: 2)
      s.shadowColor = colors.blackHintText
      s.shadowOpacity = 0.62
      s.fontColor = UIColor.white
      s.font = fonts.bold17
      s.textContainerInset = UIEdgeInsets(top: 0, left: 10, bottom: 0, right: 10)
    }

    theme.add(styleName: "BookmarksSubscriptionDiscount", forType: .light) { (s) -> (Void) in
      s.backgroundColor = colors.discountBackground
      s.cornerRadius = 6
      s.fontColor = UIColor.white
      s.font = fonts.bold17
      s.textContainerInset = UIEdgeInsets(top: 0, left: 10, bottom: 0, right: 10)
    }

    theme.add(styleName: "AllPassSubscriptionYearlyButton") { (s) -> (Void) in
      s.fontColorHighlighted = colors.white
      s.font = fonts.semibold14
      s.cornerRadius = 6
      s.fontColor = colors.allPassSubscriptionDescription
      s.backgroundColor = colors.allPassSubscriptionYearlyBackground
      s.backgroundColorHighlighted = colors.linkBlueHighlighted
    }

    theme.add(styleName: "AllPassSubscriptionMonthlyButton") { (s) -> (Void) in
      s.fontColorHighlighted = colors.white
      s.font = fonts.semibold14
      s.fontColor = colors.allPassSubscriptionMonthlyTitle
      s.cornerRadius = 6
      s.borderWidth = 3
      s.borderColor = UIColor.white.withAlphaComponent(alpha54)
      s.backgroundColorHighlighted = colors.blackDividers
      s.backgroundColor = colors.clear
    }

    theme.add(styleName: "AllPassSubscriptionRestoreButton") { (s) -> (Void) in
      s.font = fonts.medium14
      s.fontColor = colors.allPassSubscriptionTermsTitle
      s.fontColorHighlighted = colors.linkBlueHighlighted
    }

    theme.add(styleName: "AllPassSubscriptionTerms") { (s) -> (Void) in
      s.font = fonts.regular9
      s.fontColor = colors.allPassSubscriptionTermsTitle
    }

    theme.add(styleName: "AllPassSubscriptionTermsButton") { (s) -> (Void) in
      s.font = fonts.regular9
      s.fontColor = colors.allPassSubscriptionTermsTitle
    }

    theme.add(styleName: "AllPassSubscriptionTitleView") { (s) -> (Void) in
      s.backgroundColor = colors.clear
      s.cornerRadius = 10
      s.clip = true
    }

    theme.add(styleName: "AllPassSubscriptionTitle") { (s) -> (Void) in
      s.font = fonts.regular17
      s.fontColor = colors.allPassSubscriptionTitle
    }

    theme.add(styleName: "AllPassSubscriptionSubTitle") { (s) -> (Void) in
      s.font = fonts.heavy20
      s.fontColor = colors.allPassSubscriptionSubTitle
    }

    theme.add(styleName: "AllPassSubscriptionDescription1") { (s) -> (Void) in
      s.font = UIScreen.main.bounds.width > 320 ? fonts.heavy38 : fonts.heavy32
      s.fontColor = colors.allPassSubscriptionDescription
    }

    theme.add(styleName: "AllPassSubscriptionDescription2") { (s) -> (Void) in
      s.fontColor = colors.allPassSubscriptionDescription
    }

    theme.add(styleName: "AllPassSubscriptionDiscount") { (s) -> (Void) in
      s.shadowRadius = 4
      s.shadowOffset = CGSize(width: 0, height: 2)
      s.shadowColor = colors.blackHintText
      s.shadowOpacity = 0.62
      s.cornerRadius = 6
      s.font = fonts.heavy17
      s.fontColor = colors.discountText
      s.backgroundColor = colors.allPassSubscriptionDiscountBackground
      s.textContainerInset = UIEdgeInsets(top: 0, left: 10, bottom: 0, right: 10)
    }
  }
}
