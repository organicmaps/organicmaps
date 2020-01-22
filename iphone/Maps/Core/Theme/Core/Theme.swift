@objc class Theme: NSObject {
  enum ThemeType {
    case dark
    case light
  }
  typealias StyleName = String
  typealias Resolver = ((Style) -> (Void))

  @objc let colors: IColors
  @objc let fonts: IFonts
  private var themeType: ThemeType
  private var components: [StyleName: Style] = [:]
  private var resolvers: [StyleName: Resolver] = [:]
  private var dependencies: [StyleName: StyleName] = [:]

  init (type: ThemeType, colors: IColors, fonts: IFonts) {
    self.colors = colors
    self.fonts = fonts
    self.themeType = type
    super.init()
    self.register()
  }

  func registerStyleSheet<U: IStyleSheet> (_ type: U.Type) {
    U.register(theme: self, colors: colors, fonts: fonts)
  }

  func add(styleName: StyleName, _ resolver:@escaping Resolver) {
    resolvers[styleName] = resolver
  }

  func add(styleName: StyleName, from: StyleName, _ resolver:@escaping Resolver) {
    resolvers[styleName] = resolver
    dependencies[styleName] = from
  }

  func add(styleName: StyleName,  forType: ThemeType, _ resolver:@escaping Resolver) {
    guard themeType == forType else {
      return
    }
    resolvers[styleName] = resolver
  }

  func add(styleName: StyleName, from: StyleName, forType: ThemeType, _ resolver:@escaping Resolver) {
    guard themeType == forType else {
      return
    }
    resolvers[styleName] = resolver
    dependencies[styleName] = from
  }

  func get(_ styleName: StyleName) -> [Style] {
    let styleNames = styleName.split(separator: ":")
    var result = [Style]()
    for name in styleNames {
      let strName = String(name)
      if let style = components[strName] {
        result.append(style)
      } else if let resolver = resolvers[strName] {
        let style = Style()
        resolver(style)
        if let dependency = dependencies[strName] {
          style.append(self.get(dependency))
        }
        result.append(style)
      } else {
        assertionFailure("Style Not found:\(name)")
      }
    }
    return result
  }

  func register() {
    fatalError("You should register stylesheets in subclass")
  }
}
