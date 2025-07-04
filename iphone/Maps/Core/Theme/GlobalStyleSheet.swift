enum GlobalStyleSheet: String, CaseIterable {
  case tableView = "TableView"
  case tableCell = "TableCell"
  case tableViewCell = "MWMTableViewCell"
  case defaultTableViewCell
  case tableViewHeaderFooterView = "TableViewHeaderFooterView"
  case defaultSearchBar
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
  case flatGrayTransButton = "FlatGrayTransButton"
  case flatPrimaryTransButton = "FlatPrimaryTransButton"
  case flatRedTransButton = "FlatRedTransButton"
  case flatRedTransButtonBig = "FlatRedTransButtonBig"
  case flatRedButton = "FlatRedButton"
  case moreButton = "MoreButton"
  case editButton = "EditButton"
  case rateAppButton = "RateAppButton"
  case termsOfUseLinkText = "TermsOfUseLinkText"
  case termsOfUseGrayButton = "TermsOfUseGrayButton"
  case badge = "Badge"
  case blue = "MWMBlue"
  case black = "MWMBlack"
  case other = "MWMOther"
  case gray = "MWMGray"
  case separator = "MWMSeparator"
  case white = "MWMWhite"
  case datePickerView = "DatePickerView"
  case valueStepperView = "ValueStepperView"
  case grabber
  case modalSheetBackground
  case modalSheetContent
  case toastBackground
  case toastLabel
}

extension GlobalStyleSheet: IStyleSheet {
  func styleResolverFor(colors: IColors, fonts: IFonts) -> Theme.StyleResolver {
    switch self {
    case .tableView:
      return .add { s in
        s.backgroundColor = colors.white
        s.separatorColor = colors.blackDividers
        s.exclusions = [String(describing: UIDatePicker.self)]
      }
    case .tableCell:
      return .add { s in
        s.backgroundColor = colors.white
        s.fontColor = colors.blackPrimaryText
        s.tintColor = colors.linkBlue
        s.fontColorDetailed = colors.blackSecondaryText
        s.backgroundColorSelected = colors.pressBackground
        s.exclusions = [String(describing: UIDatePicker.self), "_UIActivityUserDefaultsActivityCell"]
      }
    case .tableViewCell:
      return .addFrom(Self.tableCell) { s in
      }
    case .defaultTableViewCell:
      return .add { s in
        s.backgroundColor = colors.white
      }
    case .tableViewHeaderFooterView:
      return .add { s in
        s.font = fonts.medium14
        s.fontColor = colors.blackSecondaryText
      }
    case .defaultSearchBar:
      return .add { s in
        s.backgroundColor = colors.pressBackground
        s.barTintColor = colors.clear
        s.fontColor = colors.blackPrimaryText
        s.fontColorDetailed = UIColor.white
        s.tintColor = colors.blackSecondaryText
      }
    case .searchBar:
      return .add { s in
        s.backgroundColor = colors.white
        s.barTintColor = colors.primary
        s.fontColor = colors.blackPrimaryText
        s.fontColorDetailed = UIColor.white
        s.tintColor = colors.blackSecondaryText
      }
    case .navigationBar:
      return .add { s in
        s.barTintColor = colors.primary
        s.tintColor = colors.whitePrimaryText
        s.backgroundImage = UIImage()
        s.shadowImage = UIImage()
        s.font = fonts.header
        s.fontColor = colors.whitePrimaryText
      }
    case .navigationBarItem:
      return .add { s in
        s.font = fonts.regular18
        s.fontColor = colors.whitePrimaryText
        s.fontColorDisabled = UIColor.lightGray
        s.fontColorHighlighted = colors.whitePrimaryTextHighlighted
        s.tintColor = colors.whitePrimaryText
      }
    case .checkmark:
      return .add { s in
        s.onTintColor = colors.linkBlue
        s.offTintColor = colors.blackHintText
      }
    case .switch:
      return .add { s in
        s.onTintColor = colors.linkBlue
      }
    case .pageControl:
      return .add { s in
        s.pageIndicatorTintColor = colors.blackHintText
        s.currentPageIndicatorTintColor = colors.blackSecondaryText
        s.backgroundColor = colors.white
      }
    case .starRatingView:
      return .add { s in
        s.onTintColor = colors.ratingYellow
        s.offTintColor = colors.blackDividers
      }
    case .difficultyView:
      return .add { s in
        s.colors = [colors.blackSecondaryText, colors.ratingGreen, colors.ratingYellow, colors.ratingRed]
        s.offTintColor = colors.blackSecondaryText
        s.backgroundColor = colors.clear
      }
    case .divider:
      return .add { s in
        s.backgroundColor = colors.blackDividers
      }
    case .solidDivider:
      return .add { s in
        s.backgroundColor = colors.solidDividers
      }
    case .background:
      return .add { s in
        s.backgroundColor = colors.white
        s.backgroundColorSelected = colors.pressBackground
      }
    case .pressBackground:
      return .add { s in
        s.backgroundColor = colors.pressBackground
      }
    case .primaryBackground:
      return .add { s in
        s.backgroundColor = colors.primary
      }
    case .secondaryBackground:
      return .add { s in
        s.backgroundColor = colors.secondary
      }
    case .menuBackground:
      return .add { s in
        s.backgroundColor = colors.menuBackground
      }
    case .bottomTabBarButton:
      return .add { s in
        s.backgroundColor = colors.tabBarButtonBackground
        s.tintColor = colors.blackSecondaryText
        s.coloring = MWMButtonColoring.black
        s.cornerRadius = .buttonDefault
        s.imageContainerInsets = UIEdgeInsets(top: 8, left: 8, bottom: 8, right: 8)
        s.shadowColor = UIColor(0,0,0,alpha20)
        s.shadowOpacity = 1
        s.shadowOffset = CGSize(width: 0, height: 1)
        s.onTintColor = .red
      }
    case .trackRecordingWidgetButton:
      return .addFrom(Self.bottomTabBarButton) { s in
        s.cornerRadius = .custom(23)
        s.coloring = .red
      }
    case .blackOpaqueBackground:
      return .add { s in
        s.backgroundColor = colors.blackOpaque
      }
    case .blueBackground:
      return .add { s in
        s.backgroundColor = colors.linkBlue
      }
    case .fadeBackground:
      return .add { s in
        s.backgroundColor = colors.fadeBackground
      }
    case .errorBackground:
      return .add { s in
        s.backgroundColor = colors.errorPink
      }
    case .blackStatusBarBackground:
      return .add { s in
        s.backgroundColor = colors.blackStatusBarBackground
      }
    case .presentationBackground:
      return .add { s in
        s.backgroundColor = UIColor.black.withAlphaComponent(alpha40)
      }
    case .clearBackground:
      return .add { s in
        s.backgroundColor = colors.clear
      }
    case .border:
      return .add { s in
        s.backgroundColor = colors.border
      }
    case .tabView:
      return .add { s in
        s.backgroundColor = colors.white
        s.barTintColor = colors.white
        s.tintColor = colors.linkBlue
        s.fontColor = colors.blackSecondaryText
        s.fontColorHighlighted = colors.linkBlue
        s.font = fonts.medium14
      }
    case .dialogView:
      return .add { s in
        s.cornerRadius = .buttonDefault
        s.shadowRadius = 2
        s.shadowColor = UIColor(0,0,0,alpha26)
        s.shadowOpacity = 1
        s.shadowOffset = CGSize(width: 0, height: 1)
        s.backgroundColor = colors.white
        s.clip = true
      }
    case .alertView:
      return .add { s in
        s.cornerRadius = .modalSheet
        s.shadowRadius = 6
        s.shadowColor = UIColor(0,0,0,alpha20)
        s.shadowOpacity = 1
        s.shadowOffset = CGSize(width: 0, height: 3)
        s.backgroundColor = colors.alertBackground
        s.clip = true
      }
    case .alertViewTextFieldContainer:
      return .add { s in
        s.borderColor = colors.blackDividers
        s.borderWidth = 0.5
        s.backgroundColor = colors.white
      }
    case .alertViewTextField:
      return .add { s in
        s.font = fonts.regular14
        s.fontColor = colors.blackPrimaryText
        s.tintColor = colors.blackSecondaryText
      }
    case .searchStatusBarView:
      return .add { s in
        s.backgroundColor = colors.primary
        s.shadowRadius = 2
        s.shadowColor = colors.blackDividers
        s.shadowOpacity = 1
        s.shadowOffset = CGSize(width: 0, height: 0)
      }
    case .flatNormalButton:
      return .add { s in
        s.font = fonts.medium14
        s.cornerRadius = .buttonDefault
        s.clip = true
        s.fontColor = colors.whitePrimaryText
        s.backgroundColor = colors.linkBlue
        s.fontColorHighlighted = colors.whitePrimaryTextHighlighted
        s.fontColorDisabled = colors.whitePrimaryTextHighlighted
        s.backgroundColorHighlighted = colors.linkBlueHighlighted
      }
    case .flatNormalButtonBig:
      return .addFrom(Self.flatNormalButton) { s in
        s.font = fonts.semibold16
        s.cornerRadius = .buttonDefaultBig
        s.backgroundColor = colors.linkBlue
        s.backgroundColorDisabled = colors.linkBlueHighlighted
      }
    case .flatNormalTransButton:
      return .add { s in
        s.font = fonts.medium14
        s.cornerRadius = .buttonDefault
        s.clip = true
        s.fontColor = colors.linkBlue
        s.backgroundColor = colors.clear
        s.fontColorHighlighted = colors.linkBlueHighlighted
        s.fontColorDisabled = colors.blackHintText
        s.backgroundColorHighlighted = colors.clear
      }
    case .flatNormalTransButtonBig:
      return .addFrom(Self.flatNormalTransButton) { s in
        s.font = fonts.regular17
      }
    case .flatGrayTransButton:
      return .add { s in
        s.font = fonts.medium14
        s.fontColor = colors.blackSecondaryText
        s.backgroundColor = colors.clear
        s.fontColorHighlighted = colors.linkBlueHighlighted
      }
    case .flatPrimaryTransButton:
      return .add { s in
        s.fontColor = colors.blackPrimaryText
        s.backgroundColor = colors.clear
        s.fontColorHighlighted = colors.linkBlueHighlighted
      }
    case .flatRedTransButton:
      return .add { s in
        s.font = fonts.medium14
        s.fontColor = colors.red
        s.backgroundColor = colors.clear
        s.fontColorHighlighted = colors.red
      }
    case .flatRedTransButtonBig:
      return .add { s in
        s.font = fonts.regular17
        s.fontColor = colors.red
        s.backgroundColor = colors.clear
        s.fontColorHighlighted = colors.red
      }
    case .flatRedButton:
      return .add { s in
        s.font = fonts.medium14
        s.cornerRadius = .buttonDefault
        s.fontColor = colors.whitePrimaryText
        s.backgroundColor = colors.buttonRed
        s.fontColorHighlighted = colors.buttonRedHighlighted
      }
    case .moreButton:
      return .add { s in
        s.fontColor = colors.linkBlue
        s.fontColorHighlighted = colors.linkBlueHighlighted
        s.backgroundColor = colors.clear
        s.font = fonts.regular16
      }
    case .editButton:
      return .add { s in
        s.font = fonts.regular14
        s.fontColor = colors.linkBlue
        s.cornerRadius = .buttonDefault
        s.borderColor = colors.linkBlue
        s.borderWidth = 1
        s.fontColorHighlighted = colors.linkBlueHighlighted
        s.backgroundColor = colors.clear
      }
    case .rateAppButton:
      return .add { s in
        s.font = fonts.medium17
        s.fontColor = colors.linkBlue
        s.fontColorHighlighted = colors.white
        s.borderColor = colors.linkBlue
        s.cornerRadius = .buttonDefault
        s.borderWidth = 1
        s.backgroundColor = colors.clear
        s.backgroundColorHighlighted = colors.linkBlue
      }
    case .termsOfUseLinkText:
      return .add { s in
        s.font = fonts.regular16
        s.fontColor = colors.blackPrimaryText

        s.linkAttributes = [NSAttributedString.Key.font: fonts.regular16,
                            NSAttributedString.Key.foregroundColor: colors.linkBlue,
                            NSAttributedString.Key.underlineColor: UIColor.clear]
        s.textContainerInset = UIEdgeInsets(top: 0, left: 0, bottom: 0, right: 0)
      }
    case .termsOfUseGrayButton:
      return .add { s in
        s.font = fonts.medium10
        s.fontColor = colors.blackSecondaryText
        s.fontColorHighlighted = colors.blackHintText
      }
    case .badge:
      return .add { s in
        s.round = true
        s.backgroundColor = colors.downloadBadgeBackground
      }
    case .blue:
      return .add { s in
        s.tintColor = colors.linkBlue
        s.coloring = MWMButtonColoring.blue
      }
    case .black:
      return .add { s in
        s.tintColor = colors.blackSecondaryText
        s.coloring = MWMButtonColoring.black
      }
    case .other:
      return .add { s in
        s.tintColor = colors.white
        s.coloring = MWMButtonColoring.other
      }
    case .gray:
      return .add { s in
        s.tintColor = colors.blackHintText
        s.coloring = MWMButtonColoring.gray
      }
    case .separator:
      return .add { s in
        s.tintColor = colors.blackDividers
        s.coloring = MWMButtonColoring.black
      }
    case .white:
      return .add { s in
        s.tintColor = colors.white
        s.coloring = MWMButtonColoring.white
      }
    case .datePickerView:
      return .add { s in
        s.backgroundColor = colors.white
        s.fontColor = colors.blackPrimaryText
        s.fontColorSelected = colors.whitePrimaryText
        s.backgroundColorSelected = colors.linkBlue
        s.backgroundColorHighlighted = colors.linkBlueHighlighted
        s.fontColorDisabled = colors.blackSecondaryText
      }
    case .valueStepperView:
      return .add { s in
        s.font = fonts.regular16
        s.fontColor = colors.blackPrimaryText
        s.coloring = MWMButtonColoring.blue
      }
    case .grabber:
      return .addFrom(Self.background) { s in
        s.cornerRadius = .grabber
      }
    case .modalSheetBackground:
      return .add { s in
        s.backgroundColor = colors.white
        s.shadowColor = UIColor.black
        s.shadowOffset = CGSize(width: 0, height: 1)
        s.shadowOpacity = 0.3
        s.shadowRadius = 6
        s.cornerRadius = .modalSheet
        s.clip = false
        s.maskedCorners = isiPad ? [] : [.layerMinXMinYCorner, .layerMaxXMinYCorner]
      }
    case .modalSheetContent:
      return .addFrom(Self.modalSheetBackground) { s in
        s.backgroundColor = colors.clear
        s.clip = true
      }
    case .toastBackground:
      return .add { s in
        s.cornerRadius = .modalSheet
        s.clip = true
      }
    case .toastLabel:
      return .add { s in
        s.font = fonts.regular16
        s.fontColor = colors.whitePrimaryText
        s.textAlignment = .center
      }
    }
  }
}
