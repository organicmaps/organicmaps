enum GlobalStyleSheet: String, CaseIterable {
  case tableView = "TableView"
  case tableViewCell = "MWMTableViewCell"
  case defaultTableViewCell
  case noStyleTableViewCell
  case tableViewHeaderFooterView = "TableViewHeaderFooterView"
  case searchBar = "SearchBar"
  case navigationBar = "NavigationBar"
  case navigationBarItem = "NavigationBarItem"
  case checkmark = "Checkmark"
  case `switch` = "Switch"
  case pageControl = "PageControl"
  case starRatingView = "StarRatingView"
  case difficultyView = "DifficultyView"
  case divider = "Divider"
  case solidDivider = "SolidDivider"
  case background = "Background"
  case pressBackground = "PressBackground"
  case primaryBackground = "PrimaryBackground"
  case secondaryBackground = "SecondaryBackground"
  case menuBackground = "MenuBackground"
  case bottomTabBarButton = "BottomTabBarButton"
  case trackRecordingWidgetButton = "TrackRecordingWidgetButton"
  case blackOpaqueBackground = "BlackOpaqueBackground"
  case blueBackground = "BlueBackground"
  case fadeBackground = "FadeBackground"
  case errorBackground = "ErrorBackground"
  case blackStatusBarBackground = "BlackStatusBarBackground"
  case presentationBackground = "PresentationBackground"
  case clearBackground = "ClearBackground"
  case border = "Border"
  case tabView = "TabView"
  case dialogView = "DialogView"
  case alertView = "AlertView"
  case alertViewTextFieldContainer = "AlertViewTextFieldContainer"
  case alertViewTextField = "AlertViewTextField"
  case searchStatusBarView = "SearchStatusBarView"
  case flatNormalButton = "FlatNormalButton"
  case flatNormalButtonBig = "FlatNormalButtonBig"
  case flatNormalTransButton = "FlatNormalTransButton"
  case flatNormalTransButtonBig = "FlatNormalTransButtonBig"
  case flatNormalGrayButtonBig
  case flatGrayTransButton = "FlatGrayTransButton"
  case flatPrimaryTransButton = "FlatPrimaryTransButton"
  case flatRedTransButton = "FlatRedTransButton"
  case flatRedTransButtonBig = "FlatRedTransButtonBig"
  case flatRedButtonBig
  case moreButton = "MoreButton"
  case editButton = "EditButton"
  case rateAppButton = "RateAppButton"
  case termsOfUseLinkText = "TermsOfUseLinkText"
  case termsOfUseGrayButton = "TermsOfUseGrayButton"
  case badge = "Badge"
  case blue = "MWMBlue"
  case black = "MWMBlack"
  case red
  case other = "MWMOther"
  case gray = "MWMGray"
  case separator = "MWMSeparator"
  case white = "MWMWhite"
  case valueStepperView = "ValueStepperView"
  case grabber
  case modalSheetBackground
  case modalSheetContent
  case sideMenuBackground
  case sideMenuContent
  case toastBackground
  case toastLabel
  case crowdfundingButton
}

extension GlobalStyleSheet: IStyleSheet {
  func styleResolverFor(fonts: IFonts) -> Theme.StyleResolver {
    switch self {
    case .tableView:
      return .add { s in
        s.backgroundColor = .whitePrimary
        s.separatorColor = .blackDividers
        s.exclusions = [String(describing: UIDatePicker.self)]
      }
    case .tableViewCell:
      return .add { s in
        s.backgroundColor = .whitePrimary
        s.fontColor = .blackPrimaryText
        s.tintColor = .linkBlue
        s.fontColorDetailed = .blackSecondaryText
        s.backgroundColorSelected = .pressBackground
        s.exclusions = [String(describing: UIDatePicker.self), "_UIActivityUserDefaultsActivityCell"]
      }
    case .defaultTableViewCell:
      return .add { s in
        s.backgroundColor = .whitePrimary
      }
    case .noStyleTableViewCell:
      return .add { _ in
        // configure table view cell manually
      }
    case .tableViewHeaderFooterView:
      return .add { s in
        s.font = fonts.medium14
        s.fontColor = .blackSecondaryText
      }
    case .searchBar:
      return .add { s in
        if #available(iOS 26.0, *) {
        } else {
          s.backgroundColor = .whitePrimary
          s.barTintColor = .greenPrimary
          s.fontColor = .blackPrimaryText
          s.fontColorDetailed = UIColor.white
          s.tintColor = .blackSecondaryText
        }
      }
    case .navigationBar:
      return .add { s in
        s.barTintColor = .greenPrimary
        s.tintColor = .whitePrimaryText
        s.backgroundImage = UIImage()
        s.shadowImage = UIImage()
        s.font = fonts.header
        s.fontColor = .whitePrimaryText
      }
    case .navigationBarItem:
      return .add { s in
        s.font = fonts.regular18
        s.fontColor = .whitePrimaryText
        s.fontColorDisabled = UIColor.lightGray
        s.fontColorHighlighted = .whitePrimaryTextHighlighted
        s.tintColor = .whitePrimaryText
      }
    case .checkmark:
      return .add { s in
        s.onTintColor = .linkBlue
        s.offTintColor = .blackHintText
      }
    case .switch:
      return .add { s in
        s.onTintColor = .linkBlue
      }
    case .pageControl:
      return .add { s in
        s.pageIndicatorTintColor = .blackHintText
        s.currentPageIndicatorTintColor = .blackSecondaryText
        s.backgroundColor = .whitePrimary
      }
    case .starRatingView:
      return .add { s in
        s.onTintColor = .ratingYellow
        s.offTintColor = .blackDividers
      }
    case .difficultyView:
      return .add { s in
        s.colors = [.blackSecondaryText, .ratingGreen, .ratingYellow, .ratingRed]
        s.offTintColor = .blackSecondaryText
        s.backgroundColor = .clear
      }
    case .divider:
      return .add { s in
        s.backgroundColor = .blackDividers
      }
    case .solidDivider:
      return .add { s in
        s.backgroundColor = .solidDividers
      }
    case .background:
      return .add { s in
        s.backgroundColor = .whitePrimary
        s.backgroundColorSelected = .pressBackground
      }
    case .pressBackground:
      return .add { s in
        s.backgroundColor = .pressBackground
      }
    case .primaryBackground:
      return .add { s in
        s.backgroundColor = .greenPrimary
      }
    case .secondaryBackground:
      return .add { s in
        s.backgroundColor = .greenSecondary
      }
    case .menuBackground:
      return .add { s in
        s.backgroundColor = .menuBackground
      }
    case .bottomTabBarButton:
      return .add { s in
        s.backgroundColor = .tabBarButtonBackground
        s.tintColor = .blackSecondaryText
        s.coloring = MWMButtonColoring.black
        s.cornerRadius = .buttonDefault
        s.imageContainerInsets = UIEdgeInsets(top: 8, left: 8, bottom: 8, right: 8)
        s.shadowColor = .shadow
        s.shadowOpacity = 1
        s.shadowOffset = CGSize(width: 0, height: 1)
        s.onTintColor = .redPrimary
      }
    case .trackRecordingWidgetButton:
      return .addFrom(Self.bottomTabBarButton) { s in
        s.cornerRadius = .custom(23)
        s.coloring = .red
      }
    case .blackOpaqueBackground:
      return .add { s in
        s.backgroundColor = .blackOpaque
      }
    case .blueBackground:
      return .add { s in
        s.backgroundColor = .linkBlue
      }
    case .fadeBackground:
      return .add { s in
        s.backgroundColor = .fadeBackground
      }
    case .errorBackground:
      return .add { s in
        s.backgroundColor = .errorPink
      }
    case .blackStatusBarBackground:
      return .add { s in
        s.backgroundColor = .blackStatusBarBackground
      }
    case .presentationBackground:
      return .add { s in
        s.backgroundColor = UIColor.black.withAlphaComponent(0.4)
      }
    case .clearBackground:
      return .add { s in
        s.backgroundColor = .clear
      }
    case .border:
      return .add { s in
        s.backgroundColor = .border
      }
    case .tabView:
      return .add { s in
        s.backgroundColor = .whitePrimary
        s.barTintColor = .whitePrimary
        s.tintColor = .linkBlue
        s.fontColor = .blackSecondaryText
        s.fontColorHighlighted = .linkBlue
        s.font = fonts.medium14
      }
    case .dialogView:
      return .add { s in
        s.cornerRadius = .buttonDefault
        s.shadowRadius = 2
        s.shadowColor = .shadow
        s.shadowOpacity = 1
        s.shadowOffset = CGSize(width: 0, height: 1)
        s.backgroundColor = .whitePrimary
        s.clip = true
      }
    case .alertView:
      return .add { s in
        s.cornerRadius = .modalSheet
        s.shadowRadius = 6
        s.shadowColor = .shadow
        s.shadowOpacity = 1
        s.shadowOffset = CGSize(width: 0, height: 3)
        s.backgroundColor = .alertBackground
        s.clip = true
      }
    case .alertViewTextFieldContainer:
      return .add { s in
        s.borderColor = .blackDividers
        s.borderWidth = 0.5
        s.backgroundColor = .whitePrimary
      }
    case .alertViewTextField:
      return .add { s in
        s.font = fonts.regular14
        s.fontColor = .blackPrimaryText
        s.tintColor = .blackSecondaryText
      }
    case .searchStatusBarView:
      return .add { s in
        s.backgroundColor = .greenPrimary
        s.shadowRadius = 2
        s.shadowColor = .blackDividers
        s.shadowOpacity = 1
        s.shadowOffset = CGSize(width: 0, height: 0)
      }
    case .flatNormalButton:
      return .add { s in
        s.font = fonts.medium14
        s.cornerRadius = .buttonDefault
        s.clip = true
        s.fontColor = .whitePrimaryText
        s.coloring = .whiteText
        s.tintColor = .whitePrimaryText
        s.backgroundColor = .linkBlue
        s.fontColorHighlighted = .whitePrimaryTextHighlighted
        s.fontColorDisabled = .whitePrimaryTextHighlighted
        s.backgroundColorHighlighted = .linkBlueHighlighted
      }
    case .flatNormalButtonBig:
      return .addFrom(Self.flatNormalButton) { s in
        s.font = fonts.semibold16
        s.cornerRadius = .buttonDefaultBig
        s.backgroundColor = .linkBlue
        s.backgroundColorDisabled = .linkBlueHighlighted
      }
    case .crowdfundingButton:
      return .addFrom(Self.flatNormalButtonBig) { s in
        s.font = fonts.semibold16
        s.fontColor = UIColor(fromHexString: "500000")
        s.cornerRadius = .buttonDefaultBig
        s.backgroundColor = .ratingYellow
      }
    case .flatNormalTransButton:
      return .add { s in
        s.font = fonts.medium14
        s.cornerRadius = .buttonDefault
        s.clip = true
        s.fontColor = .linkBlue
        s.tintColor = .linkBlue
        s.backgroundColor = .clear
        s.fontColorHighlighted = .linkBlueHighlighted
        s.fontColorDisabled = .blackHintText
        s.backgroundColorHighlighted = .clear
      }
    case .flatNormalTransButtonBig:
      return .addFrom(Self.flatNormalTransButton) { s in
        s.font = fonts.regular17
      }
    case .flatNormalGrayButtonBig:
      return .add { s in
        s.font = fonts.medium15
        s.cornerRadius = .buttonDefaultBig
        s.clip = true
        s.fontColor = .linkBlue
        s.tintColor = .linkBlue
        s.backgroundColor = .pressBackground
        s.fontColorHighlighted = .linkBlueHighlighted
        s.fontColorDisabled = .blackSecondaryText
        s.tintColorDisabled = .blackSecondaryText
        s.backgroundColorHighlighted = .blackDividers
      }
    case .flatGrayTransButton:
      return .add { s in
        s.font = fonts.medium14
        s.fontColor = .blackSecondaryText
        s.backgroundColor = .clear
        s.fontColorHighlighted = .linkBlueHighlighted
      }
    case .flatPrimaryTransButton:
      return .add { s in
        s.fontColor = .blackPrimaryText
        s.backgroundColor = .clear
        s.fontColorHighlighted = .linkBlueHighlighted
      }
    case .flatRedTransButton:
      return .add { s in
        s.font = fonts.medium14
        s.fontColor = .redPrimary
        s.backgroundColor = .clear
        s.fontColorHighlighted = .redPrimary
      }
    case .flatRedTransButtonBig:
      return .add { s in
        s.font = fonts.regular17
        s.fontColor = .redPrimary
        s.backgroundColor = .clear
        s.fontColorHighlighted = .redPrimary
      }
    case .flatRedButtonBig:
      return .add { s in
        s.font = fonts.semibold16
        s.cornerRadius = .buttonDefaultBig
        s.fontColor = .whitePrimaryText
        s.backgroundColor = .buttonRed
        s.fontColorHighlighted = .buttonRedHighlighted
      }
    case .moreButton:
      return .add { s in
        s.fontColor = .linkBlue
        s.fontColorHighlighted = .linkBlueHighlighted
        s.backgroundColor = .clear
        s.font = fonts.regular16
      }
    case .editButton:
      return .add { s in
        s.font = fonts.regular14
        s.fontColor = .linkBlue
        s.cornerRadius = .buttonDefault
        s.borderColor = .linkBlue
        s.borderWidth = 1
        s.fontColorHighlighted = .linkBlueHighlighted
        s.backgroundColor = .clear
      }
    case .rateAppButton:
      return .add { s in
        s.font = fonts.medium17
        s.fontColor = .linkBlue
        s.fontColorHighlighted = .whitePrimary
        s.borderColor = .linkBlue
        s.cornerRadius = .buttonDefault
        s.borderWidth = 1
        s.backgroundColor = .clear
        s.backgroundColorHighlighted = .linkBlue
      }
    case .termsOfUseLinkText:
      return .add { s in
        s.font = fonts.regular16
        s.fontColor = .blackPrimaryText

        s.linkAttributes = [NSAttributedString.Key.font: fonts.regular16,
                            NSAttributedString.Key.foregroundColor: UIColor.linkBlue,
                            NSAttributedString.Key.underlineColor: UIColor.clear]
        s.textContainerInset = UIEdgeInsets(top: 0, left: 0, bottom: 0, right: 0)
      }
    case .termsOfUseGrayButton:
      return .add { s in
        s.font = fonts.medium10
        s.fontColor = .blackSecondaryText
        s.fontColorHighlighted = .blackHintText
      }
    case .badge:
      return .add { s in
        s.round = true
        s.backgroundColor = .downloadBadgeBackground
      }
    case .blue:
      return .add { s in
        s.tintColor = .linkBlue
        s.coloring = MWMButtonColoring.blue
      }
    case .black:
      return .add { s in
        s.tintColor = .blackSecondaryText
        s.coloring = MWMButtonColoring.black
      }
    case .red:
      return .add { s in
        s.tintColor = .redPrimary
        s.coloring = MWMButtonColoring.red
      }
    case .other:
      return .add { s in
        s.tintColor = .whitePrimary
        s.coloring = MWMButtonColoring.other
      }
    case .gray:
      return .add { s in
        s.tintColor = .blackHintText
        s.coloring = MWMButtonColoring.gray
      }
    case .separator:
      return .add { s in
        s.tintColor = .blackDividers
        s.coloring = MWMButtonColoring.black
      }
    case .white:
      return .add { s in
        s.tintColor = .whitePrimary
        s.coloring = MWMButtonColoring.white
      }
    case .valueStepperView:
      return .add { s in
        s.font = fonts.regular16
        s.fontColor = .blackPrimaryText
        s.coloring = MWMButtonColoring.blue
      }
    case .grabber:
      return .addFrom(Self.divider) { s in
        s.cornerRadius = .grabber
      }
    case .modalSheetBackground:
      return .add { s in
        s.backgroundColor = .whitePrimary
        s.shadowColor = UIColor.black
        s.shadowOffset = CGSize(width: 0, height: 1)
        s.shadowOpacity = 0.3
        s.shadowRadius = 6
        s.cornerRadius = .modalSheet
        s.clip = false
        s.maskedCorners = [.layerMinXMinYCorner, .layerMaxXMinYCorner]
      }
    case .modalSheetContent:
      return .addFrom(Self.modalSheetBackground) { s in
        s.backgroundColor = .clear
        s.clip = true
      }
    case .sideMenuBackground:
      return .addFrom(Self.modalSheetBackground) { s in
        s.maskedCorners = []
      }
    case .sideMenuContent:
      return .addFrom(Self.modalSheetContent) { s in
        s.maskedCorners = []
      }
    case .toastBackground:
      return .add { s in
        s.cornerRadius = .buttonDefaultBig
        s.clip = true
      }
    case .toastLabel:
      return .add { s in
        s.font = fonts.regular16
        s.fontColor = .whitePrimaryText
        s.textAlignment = .center
      }
    }
  }
}
