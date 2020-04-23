class GuidesGalleryStyleSheet: IStyleSheet {
  private enum Const {
    static let cityColor = UIColor(red: 0.4, green: 0.225, blue: 0.75, alpha: 1)
    static let outdoorColor = UIColor(red: 0.235, green: 0.549, blue: 0.235, alpha: 1)
  }

  static func register(theme: Theme, colors: IColors, fonts: IFonts) {
    theme.add(styleName: "GuidesGalleryCell") { (s) -> (Void) in
      s.backgroundColor = colors.white
      s.cornerRadius = 4
      s.clip = true
      s.shadowColor = colors.shadow
      s.shadowOpacity = 0.25
      s.shadowRadius = 4
    }

    theme.add(styleName: "GuidesGalleryCellImage") { (s) -> (Void) in
      s.cornerRadius = 4
      s.clip = true
      s.backgroundColor = colors.blackDividers
    }

    theme.add(styleName: "GuidesGalleryCityLabel") { (s) -> (Void) in
      s.fontColor = Const.cityColor
      s.font = fonts.regular12
    }

    theme.add(styleName: "GuidesGalleryOutdoorLabel") { (s) -> (Void) in
      s.fontColor = Const.outdoorColor
      s.font = fonts.regular12
    }

    theme.add(styleName: "GuidesGalleryCityCheck") { (s) -> (Void) in
      s.tintColor = Const.cityColor
    }

    theme.add(styleName: "GuidesGalleryOutdoorCheck") { (s) -> (Void) in
      s.tintColor = Const.outdoorColor
    }

    theme.add(styleName: "GuidesGalleryShowButton") { (s) -> (Void) in
      s.cornerRadius = 14
      s.clip = true
      s.backgroundColor = colors.blackOpaque
      s.font = fonts.semibold14
      s.fontColor = colors.blackSecondaryText
    }
  }
}
