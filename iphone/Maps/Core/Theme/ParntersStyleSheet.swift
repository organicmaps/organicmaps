class PartnersStyleSheet: IStyleSheet {
  static func register(theme: Theme, colors: IColors, fonts: IFonts) {
    theme.add(styleName: "Tinkoff") { (s) -> (Void) in
      s.backgroundColor = UIColor(fromHexString: "FFDD2D")
      s.fontColor = .black
      s.font = fonts.semibold14
      s.cornerRadius = 14
      s.clip = true
    }

    theme.add(styleName: "Mts") { (s) -> (Void) in
      s.backgroundColor = UIColor(fromHexString: "E30611")
      s.fontColor = .white
      s.font = fonts.semibold14
      s.cornerRadius = 14
      s.clip = true
    }
    
    theme.add(styleName: "Skyeng") { (s) -> (Void) in
      s.backgroundColor = UIColor(fromHexString: "4287DF")
      s.fontColor = .white
      s.font = fonts.semibold14
      s.cornerRadius = 14
      s.clip = true
    }

    theme.add(styleName: "Sberbank") { (s) -> (Void) in
      s.backgroundColor = UIColor(fromHexString: "009A37")
      s.fontColor = .white
      s.font = fonts.semibold14
      s.cornerRadius = 14
      s.clip = true
    }
    
    theme.add(styleName: "Arsenal") { (s) -> (Void) in
      s.backgroundColor = UIColor(fromHexString: "93C950")
      s.fontColor = .white
      s.font = fonts.semibold14
      s.cornerRadius = 14
      s.clip = true
    }
  }
}

