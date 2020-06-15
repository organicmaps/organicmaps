class GuidesGalleryStyleSheet: IStyleSheet {
  static func register(theme: Theme, colors: IColors, fonts: IFonts) {
    theme.add(styleName: "GuidesGalleryCell") { (s) -> (Void) in
      s.backgroundColor = colors.white
      s.cornerRadius = 10
      s.clip = false
      s.shadowColor = colors.shadow
      s.shadowOpacity = 0.25
      s.shadowRadius = 4
      s.shadowOffset = CGSize(width: 0, height: 0)
    }

    theme.add(styleName: "GuidesGalleryCellImage") { (s) -> (Void) in
      s.cornerRadius = 4
      s.clip = true
      s.backgroundColor = colors.blackDividers
    }

    theme.add(styleName: "GuidesGalleryCityLabel") { (s) -> (Void) in
      s.fontColor = colors.cityColor
      s.font = fonts.regular12
    }

    theme.add(styleName: "GuidesGalleryOutdoorLabel") { (s) -> (Void) in
      s.fontColor = colors.outdoorColor
      s.font = fonts.regular12
    }

    theme.add(styleName: "GuidesGalleryCityCheck") { (s) -> (Void) in
      s.tintColor = colors.cityColor
    }

    theme.add(styleName: "GuidesGalleryOutdoorCheck") { (s) -> (Void) in
      s.tintColor = colors.outdoorColor
    }

    theme.add(styleName: "GuidesGalleryShowButton") { (s) -> (Void) in
      s.cornerRadius = 8
      s.borderWidth = 1
      s.borderColor = colors.linkBlue
      s.fontColor = colors.linkBlue
      s.backgroundColor = colors.white
      s.clip = true
      s.font = fonts.semibold14
    }
  }
}
