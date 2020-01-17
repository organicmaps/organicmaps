class Style: ExpressibleByDictionaryLiteral {
  enum Parameter: Hashable{
    case backgroundColor
    case borderColor
    case borderWidth
    case cornerRadius
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
    case separatorColor
    case pageIndicatorTintColor
    case currentPageIndicatorTintColor

    case coloring
    case colors
    case images
    case unknown
  }

  typealias Key = Parameter
  typealias Value = Any?

  var params:[Key: Value] = [:]

  required init(dictionaryLiteral elements: (Style.Parameter, Any?)...) {
    for (key, value) in elements {
      params[key] = value
    }
  }

  subscript(keyname: Key) -> Value {
    get { return self.params[keyname] != nil ? self.params[keyname]! : nil }
  }

  func append(_ style: Style){
    self.params.merge(style.params) { (a, b) -> Style.Value in
      return b
    }
  }

  func append(_ styles: [Style]){
    styles.forEach { (style) in
      self.params.merge(style.params) { (a, b) -> Style.Value in
        return b
      }
    }
  }
}


extension Style {
  var backgroundColor: UIColor? {
    get { return self[.backgroundColor] as? UIColor }
    set { self.params[.backgroundColor] = newValue }
  }

  var borderColor: UIColor? {
    get { return self[.borderColor] as? UIColor }
    set { self.params[.borderColor] = newValue }
  }

  var borderWidth: CGFloat? {
    get { return self[.borderWidth] as? CGFloat }
    set { self.params[.borderWidth] = newValue }
  }

  var cornerRadius: CGFloat? {
    get { return self[.cornerRadius] as? CGFloat }
    set { self.params[.cornerRadius] = newValue }
  }

  var shadowColor: UIColor? {
    get { return self[.shadowColor] as? UIColor }
    set { self.params[.shadowColor] = newValue }
  }

  var shadowOpacity: Float? {
    get { return self[.shadowOpacity] as? Float }
    set { self.params[.shadowOpacity] = newValue }
  }

  var shadowOffset: CGSize? {
    get { return self[.shadowOffset] as? CGSize }
    set { self.params[.shadowOffset] = newValue }
  }

  var shadowRadius: CGFloat? {
    get { return self[.shadowRadius] as? CGFloat }
    set { self.params[.shadowRadius] = newValue }
  }

  var clip: Bool? {
    get { return self[.clip] as? Bool }
    set { self.params[.clip] = newValue }
  }

  var round: Bool? {
    get { return self[.round] as? Bool }
    set { self.params[.round] = newValue }
  }

  var font: UIFont? {
    get { return self[.font] as? UIFont }
    set { self.params[.font] = newValue }
  }

  var fontColor: UIColor? {
    get { return self[.fontColor] as? UIColor }
    set { self.params[.fontColor] = newValue }
  }

  var fontDetailed: UIFont? {
    get { return self[.fontDetailed] as? UIFont }
    set { self.params[.fontDetailed] = newValue }
  }

  var fontColorDetailed: UIColor? {
    get { return self[.fontColorDetailed] as? UIColor }
    set { self.params[.fontColorDetailed] = newValue }
  }

  var tintColor: UIColor? {
    get { return self[.tintColor] as? UIColor }
    set { self.params[.tintColor] = newValue }
  }

  var tintColorDisabled: UIColor? {
    get { return self[.tintColorDisabled] as? UIColor }
    set { self.params[.tintColorDisabled] = newValue }
  }

  var onTintColor: UIColor? {
    get { return self[.onTintColor] as? UIColor }
    set { self.params[.onTintColor] = newValue }
  }

  var offTintColor: UIColor? {
    get { return self[.offTintColor] as? UIColor }
    set { self.params[.offTintColor] = newValue }
  }

  var image: String? {
    get { return self[.image] as? String }
    set { self.params[.image] = newValue }
  }

  var mwmImage: String? {
    get { return self[.mwmImage] as? String }
    set { self.params[.mwmImage] = newValue }
  }

  var color: UIColor? {
    get { return self[.color] as? UIColor }
    set { self.params[.color] = newValue }
  }

  var attributes: [NSAttributedString.Key : Any]? {
    get { return self[.attributes] as? [NSAttributedString.Key : Any] }
    set { self.params[.attributes] = newValue }
  }

  var linkAttributes: [NSAttributedString.Key : Any]? {
    get { return self[.linkAttributes] as? [NSAttributedString.Key : Any] }
    set { self.params[.linkAttributes] = newValue }
  }

  var backgroundImage: UIImage? {
    get { return self[.backgroundImage] as? UIImage }
    set { self.params[.backgroundImage] = newValue }
  }

  var barTintColor: UIColor? {
    get { return self[.barTintColor] as? UIColor }
    set { self.params[.barTintColor] = newValue }
  }

  var backgroundColorSelected: UIColor? {
    get { return self[.backgroundColorSelected] as? UIColor }
    set { self.params[.backgroundColorSelected] = newValue }
  }

  var backgroundColorHighlighted: UIColor? {
    get { return self[.backgroundColorHighlighted] as? UIColor }
    set { self.params[.backgroundColorHighlighted] = newValue }
  }

  var backgroundColorDisabled: UIColor? {
    get { return self[.backgroundColorDisabled] as? UIColor }
    set { self.params[.backgroundColorDisabled] = newValue }
  }

  var fontColorSelected: UIColor? {
    get { return self[.fontColorSelected] as? UIColor }
    set { self.params[.fontColorSelected] = newValue }
  }

  var fontColorHighlighted: UIColor? {
    get { return self[.fontColorHighlighted] as? UIColor }
    set { self.params[.fontColorHighlighted] = newValue }
  }

  var fontColorDisabled: UIColor? {
    get { return self[.fontColorDisabled] as? UIColor }
    set { self.params[.fontColorDisabled] = newValue }
  }

  var shadowImage: UIImage? {
    get { return self[.shadowImage] as? UIImage }
    set { self.params[.shadowImage] = newValue }
  }

  var textAlignment: NSTextAlignment? {
    get { return self[.textAlignment] as? NSTextAlignment }
    set { self.params[.textAlignment] = newValue }
  }

  var textContainerInset: UIEdgeInsets? {
    get { return self[.textContainerInset] as? UIEdgeInsets }
    set { self.params[.textContainerInset] = newValue }
  }

  var separatorColor: UIColor? {
    get { return self[.separatorColor] as? UIColor }
    set { self.params[.separatorColor] = newValue }
  }

  var pageIndicatorTintColor: UIColor? {
    get { return self[.pageIndicatorTintColor] as? UIColor }
    set { self.params[.pageIndicatorTintColor] = newValue }
  }

  var currentPageIndicatorTintColor: UIColor? {
    get { return self[.currentPageIndicatorTintColor] as? UIColor }
    set { self.params[.currentPageIndicatorTintColor] = newValue }
  }

  var colors: [UIColor]? {
    get { return self[.colors] as? [UIColor] }
    set { self.params[.colors] = newValue }
  }

  var images: [String]? {
    get { return self[.images] as? [String] }
    set { self.params[.images] = newValue }
  }

  var coloring: MWMButtonColoring? {
    get { return self[.coloring] as? MWMButtonColoring }
    set { self.params[.coloring] = newValue }
  }
}
