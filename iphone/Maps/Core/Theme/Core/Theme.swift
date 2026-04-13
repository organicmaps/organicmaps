@objc class Theme: NSObject {
  typealias StyleName = String
  typealias Resolver = (Style) -> Void

  @objc let fonts: IFonts
  private var components: [StyleName: Style] = [:]
  private var resolvers: [StyleName: Resolver] = [:]
  private var dependencies: [StyleName: StyleName] = [:]

  init(fonts: IFonts) {
    self.fonts = fonts
    super.init()
    register()
  }

  func registerStyleSheet<U: IStyleSheet>(_: U.Type) {
    U.register(theme: self, fonts: fonts)
  }

  func add(styleName: StyleName, _ resolver: @escaping Resolver) {
    resolvers[styleName] = resolver
  }

  func add(styleName: StyleName, from: StyleName, _ resolver: @escaping Resolver) {
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
          style.append(get(dependency))
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
