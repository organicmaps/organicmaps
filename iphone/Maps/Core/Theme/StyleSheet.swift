protocol StyleStringRepresentable {
  var rawValue: String { get }

  func styleResolverFor(colors: IColors, fonts: IFonts) -> Theme.StyleResolver
}

extension Theme {
  enum StyleResolver {
    case add(_ resolver: Theme.Resolver)
    case addFrom(_ from: StyleStringRepresentable, _ resolver: Theme.Resolver)
    case addFromType(_ forType: ThemeType, _ resolver: Theme.Resolver)
    case addFromForType(_ from: StyleStringRepresentable, _ forType: ThemeType, _ resolver: Theme.Resolver)
  }

  func add(_ style: StyleStringRepresentable, _ resolvingType: StyleResolver) {
    switch resolvingType {
    case .add(let resolver):
      add(styleName: style.rawValue, resolver)
    case .addFrom(let from, let resolver):
      add(styleName: style.rawValue, from: from.rawValue, resolver)
    case .addFromType(let forType, let resolver):
      add(styleName: style.rawValue, forType: forType, resolver)
    case .addFromForType(let from, let forType, let resolver):
      add(styleName: style.rawValue, from: from.rawValue, forType: forType, resolver)
    }
  }
}
