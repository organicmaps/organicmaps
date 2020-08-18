class FontStyleSheet: IStyleSheet {
  static func register(theme: Theme, colors: IColors, fonts: IFonts) {
    theme.add(styleName: "regular9") { (s) -> (Void) in
      s.font = fonts.regular9
    }
    theme.add(styleName: "regular10") { (s) -> (Void) in
      s.font = fonts.regular10
    }
    theme.add(styleName: "regular11") { (s) -> (Void) in
      s.font = fonts.regular11
    }
    theme.add(styleName: "regular12") { (s) -> (Void) in
      s.font = fonts.regular12
    }
    theme.add(styleName: "regular13") { (s) -> (Void) in
      s.font = fonts.regular13
    }
    theme.add(styleName: "regular14") { (s) -> (Void) in
      s.font = fonts.regular14
    }
    theme.add(styleName: "regular15") { (s) -> (Void) in
      s.font = fonts.regular15
    }
    theme.add(styleName: "regular16") { (s) -> (Void) in
      s.font = fonts.regular16
    }
    theme.add(styleName: "regular17") { (s) -> (Void) in
      s.font = fonts.regular17
    }
    theme.add(styleName: "regular18") { (s) -> (Void) in
      s.font = fonts.regular18
    }
    theme.add(styleName: "regular20") { (s) -> (Void) in
      s.font = fonts.regular20
    }
    theme.add(styleName: "regular24") { (s) -> (Void) in
      s.font = fonts.regular24
    }
    theme.add(styleName: "regular32") { (s) -> (Void) in
      s.font = fonts.regular32
    }
    theme.add(styleName: "regular52") { (s) -> (Void) in
      s.font = fonts.regular52
    }
    theme.add(styleName: "medium9") { (s) -> (Void) in
      s.font = fonts.medium9
    }
    theme.add(styleName: "medium10") { (s) -> (Void) in
      s.font = fonts.medium10
    }
    theme.add(styleName: "medium12") { (s) -> (Void) in
      s.font = fonts.medium12
    }
    theme.add(styleName: "medium14") { (s) -> (Void) in
      s.font = fonts.medium14
    }
    theme.add(styleName: "medium16") { (s) -> (Void) in
      s.font = fonts.medium16
    }
    theme.add(styleName: "medium17") { (s) -> (Void) in
      s.font = fonts.medium17
    }
    theme.add(styleName: "medium18") { (s) -> (Void) in
      s.font = fonts.medium18
    }
    theme.add(styleName: "medium20") { (s) -> (Void) in
      s.font = fonts.medium20
    }
    theme.add(styleName: "medium24") { (s) -> (Void) in
      s.font = fonts.medium24
    }
    theme.add(styleName: "medium28") { (s) -> (Void) in
      s.font = fonts.medium28
    }
    theme.add(styleName: "medium36") { (s) -> (Void) in
      s.font = fonts.medium36
    }
    theme.add(styleName: "medium40") { (s) -> (Void) in
      s.font = fonts.medium40
    }
    theme.add(styleName: "medium44") { (s) -> (Void) in
      s.font = fonts.medium44
    }
    theme.add(styleName: "light10") { (s) -> (Void) in
      s.font = fonts.light10
    }
    theme.add(styleName: "light12") { (s) -> (Void) in
      s.font = fonts.light12
    }
    theme.add(styleName: "light16") { (s) -> (Void) in
      s.font = fonts.light16
    }
    theme.add(styleName: "light17") { (s) -> (Void) in
      s.font = fonts.light17
    }
    theme.add(styleName: "bold12") { (s) -> (Void) in
      s.font = fonts.bold12
    }
    theme.add(styleName: "bold14") { (s) -> (Void) in
      s.font = fonts.bold14
    }
    theme.add(styleName: "bold16") { (s) -> (Void) in
      s.font = fonts.bold16
    }
    theme.add(styleName: "bold17") { (s) -> (Void) in
      s.font = fonts.bold17
    }
    theme.add(styleName: "bold18") { (s) -> (Void) in
      s.font = fonts.bold18
    }
    theme.add(styleName: "bold20") { (s) -> (Void) in
      s.font = fonts.bold20
    }
    theme.add(styleName: "bold22") { (s) -> (Void) in
      s.font = fonts.bold22
    }
    theme.add(styleName: "bold24") { (s) -> (Void) in
      s.font = fonts.bold24
    }
    theme.add(styleName: "bold28") { (s) -> (Void) in
      s.font = fonts.bold28
    }
    theme.add(styleName: "bold34") { (s) -> (Void) in
      s.font = fonts.bold34
    }
    theme.add(styleName: "bold36") { (s) -> (Void) in
      s.font = fonts.bold36
    }
    theme.add(styleName: "bold48") { (s) -> (Void) in
      s.font = fonts.bold48
    }
    theme.add(styleName: "heavy17") { (s) -> (Void) in
      s.font = fonts.heavy17
    }
    theme.add(styleName: "heavy20") { (s) -> (Void) in
      s.font = fonts.heavy20
    }
    theme.add(styleName: "heavy32") { (s) -> (Void) in
      s.font = fonts.heavy32
    }
    theme.add(styleName: "heavy38") { (s) -> (Void) in
      s.font = fonts.heavy38
    }
    theme.add(styleName: "italic16") { (s) -> (Void) in
      s.font = fonts.italic16
    }
    theme.add(styleName: "semibold12") { (s) -> (Void) in
      s.font = fonts.semibold12
    }
    theme.add(styleName: "semibold14") { (s) -> (Void) in
      s.font = fonts.semibold14
    }
    theme.add(styleName: "semibold15") { (s) -> (Void) in
      s.font = fonts.semibold15
    }
    theme.add(styleName: "semibold16") { (s) -> (Void) in
      s.font = fonts.semibold16
    }
    theme.add(styleName: "semibold18") { (s) -> (Void) in
      s.font = fonts.semibold18
    }

    theme.add(styleName: "whitePrimaryText") { (s) -> (Void) in
      s.fontColor = colors.whitePrimaryText
    }
    theme.add(styleName: "blackSecondaryText") { (s) -> (Void) in
      s.fontColor = colors.blackSecondaryText
    }
    theme.add(styleName: "blackPrimaryText") { (s) -> (Void) in
      s.fontColor = colors.blackPrimaryText
    }
    theme.add(styleName: "linkBlueText") { (s) -> (Void) in
      s.fontColor = colors.linkBlue
    }
    theme.add(styleName: "linkBlueHighlightedText") { (s) -> (Void) in
      s.fontColor = colors.linkBlueHighlighted
    }
    theme.add(styleName: "whiteText") { (s) -> (Void) in
      s.fontColor = colors.white
    }
    theme.add(styleName: "blackHintText") { (s) -> (Void) in
      s.fontColor = colors.blackHintText
    }
    theme.add(styleName: "greenText") { (s) -> (Void) in
      s.fontColor = colors.ratingGreen
    }
    theme.add(styleName: "redText") { (s) -> (Void) in
      s.fontColor = colors.red
    }
    theme.add(styleName: "buttonRedText") { (s) -> (Void) in
      s.fontColor = colors.buttonRed
    }
  }
}
