class DiscoveryStyleSheet: IStyleSheet {
  static func register(theme: Theme, colors: IColors, fonts: IFonts) {
    theme.add(styleName: "DiscoveryCell") { (s) -> (Void) in
      s.backgroundColor = colors.white
      s.cornerRadius = 8
      s.borderWidth = 1
      s.borderColor = colors.blackDividers
      s.clip = true
    }

    theme.add(styleName: "DiscoveryPopularView") { (s) -> (Void) in
      s.backgroundColor = colors.linkBlueHighlighted
      s.cornerRadius = 10
      s.borderColor = colors.blackDividers
      s.clip = true
    }

    theme.add(styleName: "DiscoveryController") { (s) -> (Void) in
      s.backgroundColor = colors.pressBackground
    }

    theme.add(styleName: "DiscoveryMoreImage") { (s) -> (Void) in
      s.mwmImage = "btn_float_more"
      s.coloring = .other
    }
  }
}
