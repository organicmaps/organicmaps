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
      return PlacePageUserDescriptionWebView(htmlString: string)
    case .plain:
      return ExpandableTextView(expandableText: text)
    }
  }
}
