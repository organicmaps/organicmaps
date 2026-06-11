protocol ExpandableTextContainer: AnyObject {
  var contentView: UIView { get }
  var onContentHeightChanged: (() -> Void)? { get set }

  func configure(with text: String)
  func updateContentHeight()
  func expandedHeight(for width: CGFloat) -> CGFloat
  func collapsedHeight(for width: CGFloat) -> CGFloat
}

extension ExpandableTextContainer {
  func updateContentHeight() {}
}

protocol ExpandableTextContainerFactory {
  static func makeContainer(for text: ExpandableText) -> any ExpandableTextContainer
}

enum PlacePageTextContainerFactory: ExpandableTextContainerFactory {
  static func makeContainer(for text: ExpandableText) -> any ExpandableTextContainer {
    ExpandableTextView(expandableText: text)
  }
}

enum PlacePageUserDescriptionContainerFactory: ExpandableTextContainerFactory {
  static func makeContainer(for text: ExpandableText) -> any ExpandableTextContainer {
    switch text {
    case .html(let string):
      // Use WKWebView only for user descriptions. OSM/Wiki descriptions stay on the lightweight text renderer
      // to avoid instantiating several independent web views on the same place page.
      return PlacePageUserDescriptionWebView(htmlString: string)
    case .plain:
      return ExpandableTextView(expandableText: text)
    }
  }
}
