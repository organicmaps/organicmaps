class Style: ExpressibleByDictionaryLiteral {
  enum Parameter: Hashable {
    case backgroundColor
    case borderColor
    case borderWidth
    case cornerRadius
    case maskedCorners
    case shadowColor
    case shadowOpacity
    case shadowOffset
    case shadowRadius
    case clip
    case round

    case font
    case fontColor
    case fontDetailed
    case fontColorDetailed
    case tintColor
    case tintColorDisabled
    case onTintColor
    case offTintColor
    case image
    case mwmImage
    case color
    case attributes
    case linkAttributes

    case backgroundImage
    case backgroundColorSelected
    case backgroundColorHighlighted
    case backgroundColorDisabled
    case fontColorSelected
    case fontColorHighlighted
    case fontColorDisabled
    case barTintColor
    case shadowImage
    case textAlignment
    case textContainerInset
    case imageContainerInsets
    case separatorColor
    case pageIndicatorTintColor
    case currentPageIndicatorTintColor

    case coloring
    case colors
    case images
    case exclusions
    case unknown

    case gridColor
    case previewSelectorColor
    case previewTintColor
    case infoBackground
  }

  typealias Key = Parameter
  typealias Value = Any?

  var params: [Key: Value] = [:]
  var isEmpty: Bool {
    params.isEmpty
  }

  required init(dictionaryLiteral elements: (Style.Parameter, Any?)...) {
    for (key, value) in elements {
      params[key] = value
    }
  }

  subscript(keyname: Key) -> Value { params[keyname] ?? nil }

  func append(_ style: Style) {
    params.merge(style.params) { a, _ -> Style.Value in
      return a
    }
  }

  func append(_ styles: [Style]) {
    for style in styles {
      params.merge(style.params) { a, _ -> Style.Value in
        return a
      }
    }
  }

  func hasExclusion(view: UIView) -> Bool {
    guard let exclusions = exclusions else {
      return false
    }
    var superView: UIView? = view
    while superView != nil {
      if exclusions.contains(String(describing: type(of: superView!))) {
        return true
      }
      superView = superView?.superview
    }
    return false
  }
}

extension Style {
  var backgroundColor: UIColor? {
    get { self[.backgroundColor] as? UIColor }
    set { params[.backgroundColor] = newValue }
  }

  var borderColor: UIColor? {
    get { self[.borderColor] as? UIColor }
    set { params[.borderColor] = newValue }
  }

  var borderWidth: CGFloat? {
    get { self[.borderWidth] as? CGFloat }
    set { params[.borderWidth] = newValue }
  }

  var cornerRadius: CornerRadius? {
    get { self[.cornerRadius] as? CornerRadius }
    set { params[.cornerRadius] = newValue }
  }

  var maskedCorners: CACornerMask? {
    get { self[.maskedCorners] as? CACornerMask }
    set { params[.maskedCorners] = newValue }
  }

  var shadowColor: UIColor? {
    get { self[.shadowColor] as? UIColor }
    set { params[.shadowColor] = newValue }
  }

  var shadowOpacity: Float? {
    get { self[.shadowOpacity] as? Float }
    set { params[.shadowOpacity] = newValue }
  }

  var shadowOffset: CGSize? {
    get { self[.shadowOffset] as? CGSize }
    set { params[.shadowOffset] = newValue }
  }

  var shadowRadius: CGFloat? {
    get { self[.shadowRadius] as? CGFloat }
    set { params[.shadowRadius] = newValue }
  }

  var clip: Bool? {
    get { self[.clip] as? Bool }
    set { params[.clip] = newValue }
  }

  var round: Bool? {
    get { self[.round] as? Bool }
    set { params[.round] = newValue }
  }

  var font: UIFont? {
    get { self[.font] as? UIFont }
    set { params[.font] = newValue }
  }

  var fontColor: UIColor? {
    get { self[.fontColor] as? UIColor }
    set { params[.fontColor] = newValue }
  }

  var fontDetailed: UIFont? {
    get { self[.fontDetailed] as? UIFont }
    set { params[.fontDetailed] = newValue }
  }

  var fontColorDetailed: UIColor? {
    get { self[.fontColorDetailed] as? UIColor }
    set { params[.fontColorDetailed] = newValue }
  }

  var tintColor: UIColor? {
    get { self[.tintColor] as? UIColor }
    set { params[.tintColor] = newValue }
  }

  var tintColorDisabled: UIColor? {
    get { self[.tintColorDisabled] as? UIColor }
    set { params[.tintColorDisabled] = newValue }
  }

  var onTintColor: UIColor? {
    get { self[.onTintColor] as? UIColor }
    set { params[.onTintColor] = newValue }
  }

  var offTintColor: UIColor? {
    get { self[.offTintColor] as? UIColor }
    set { params[.offTintColor] = newValue }
  }

  var image: String? {
    get { self[.image] as? String }
    set { params[.image] = newValue }
  }

  var mwmImage: String? {
    get { self[.mwmImage] as? String }
    set { params[.mwmImage] = newValue }
  }

  var color: UIColor? {
    get { self[.color] as? UIColor }
    set { params[.color] = newValue }
  }

  var attributes: [NSAttributedString.Key: Any]? {
    get { self[.attributes] as? [NSAttributedString.Key: Any] }
    set { params[.attributes] = newValue }
  }

  var linkAttributes: [NSAttributedString.Key: Any]? {
    get { self[.linkAttributes] as? [NSAttributedString.Key: Any] }
    set { params[.linkAttributes] = newValue }
  }

  var backgroundImage: UIImage? {
    get { self[.backgroundImage] as? UIImage }
    set { params[.backgroundImage] = newValue }
  }

  var barTintColor: UIColor? {
    get { self[.barTintColor] as? UIColor }
    set { params[.barTintColor] = newValue }
  }

  var backgroundColorSelected: UIColor? {
    get { self[.backgroundColorSelected] as? UIColor }
    set { params[.backgroundColorSelected] = newValue }
  }

  var backgroundColorHighlighted: UIColor? {
    get { self[.backgroundColorHighlighted] as? UIColor }
    set { params[.backgroundColorHighlighted] = newValue }
  }

  var backgroundColorDisabled: UIColor? {
    get { self[.backgroundColorDisabled] as? UIColor }
    set { params[.backgroundColorDisabled] = newValue }
  }

  var fontColorSelected: UIColor? {
    get { self[.fontColorSelected] as? UIColor }
    set { params[.fontColorSelected] = newValue }
  }

  var fontColorHighlighted: UIColor? {
    get { self[.fontColorHighlighted] as? UIColor }
    set { params[.fontColorHighlighted] = newValue }
  }

  var fontColorDisabled: UIColor? {
    get { self[.fontColorDisabled] as? UIColor }
    set { params[.fontColorDisabled] = newValue }
  }

  var shadowImage: UIImage? {
    get { self[.shadowImage] as? UIImage }
    set { params[.shadowImage] = newValue }
  }

  var textAlignment: NSTextAlignment? {
    get { self[.textAlignment] as? NSTextAlignment }
    set { params[.textAlignment] = newValue }
  }

  var textContainerInset: UIEdgeInsets? {
    get { self[.textContainerInset] as? UIEdgeInsets }
    set { params[.textContainerInset] = newValue }
  }

  var imageContainerInsets: UIEdgeInsets? {
    get { self[.imageContainerInsets] as? UIEdgeInsets }
    set { params[.imageContainerInsets] = newValue }
  }

  var separatorColor: UIColor? {
    get { self[.separatorColor] as? UIColor }
    set { params[.separatorColor] = newValue }
  }

  var pageIndicatorTintColor: UIColor? {
    get { self[.pageIndicatorTintColor] as? UIColor }
    set { params[.pageIndicatorTintColor] = newValue }
  }

  var currentPageIndicatorTintColor: UIColor? {
    get { self[.currentPageIndicatorTintColor] as? UIColor }
    set { params[.currentPageIndicatorTintColor] = newValue }
  }

  var colors: [UIColor]? {
    get { self[.colors] as? [UIColor] }
    set { params[.colors] = newValue }
  }

  var images: [String]? {
    get { self[.images] as? [String] }
    set { params[.images] = newValue }
  }

  var coloring: MWMButtonColoring? {
    get { self[.coloring] as? MWMButtonColoring }
    set { params[.coloring] = newValue }
  }

  var exclusions: Set<String>? {
    get { self[.exclusions] as? Set<String> }
    set { params[.exclusions] = newValue }
  }

  var gridColor: UIColor? {
    get { self[.gridColor] as? UIColor }
    set { params[.gridColor] = newValue }
  }

  var previewSelectorColor: UIColor? {
    get { self[.previewSelectorColor] as? UIColor }
    set { params[.previewSelectorColor] = newValue }
  }

  var previewTintColor: UIColor? {
    get { self[.previewTintColor] as? UIColor }
    set { params[.previewTintColor] = newValue }
  }

  var infoBackground: UIColor? {
    get { self[.infoBackground] as? UIColor }
    set { params[.infoBackground] = newValue }
  }
}
