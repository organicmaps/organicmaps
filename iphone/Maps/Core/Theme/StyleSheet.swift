protocol StyleStringRepresentable {
  var rawValue: String { get }

  func styleResolverFor(fonts: IFonts) -> Theme.StyleResolver
}

extension Theme {
  enum StyleResolver {
    case add(_ resolver: Theme.Resolver)
    case addFrom(_ from: StyleStringRepresentable, _ resolver: Theme.Resolver)
  }

  func add(_ style: StyleStringRepresentable, _ resolvingType: StyleResolver) {
    switch resolvingType {
    case .add(let resolver):
      add(styleName: style.rawValue, resolver)
    case .addFrom(let from, let resolver):
      add(styleName: style.rawValue, from: from.rawValue, resolver)
    }
  }
}
