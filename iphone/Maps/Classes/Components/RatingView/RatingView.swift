import UIKit

@IBDesignable final class RatingView: UIView {
  @IBInspectable var value: CGFloat = RatingViewSettings.Default.value {
    didSet {
      guard oldValue != value else { return }
      let clamped = min(CGFloat(settings.starsCount), max(0, value))
      if clamped == value {
        update()
      } else {
        value = clamped
      }
    }
  }

  @IBInspectable var leftText: String? {
    get { return texts[.left] }
    set { texts[.left] = newValue }
  }

  @IBInspectable var leftTextColor: UIColor {
    get { return settings.textColors[.left]! }
    set {
      settings.textColors[.left] = newValue
      update()
    }
  }

  var leftTextFont: UIFont {
    get { return settings.textFonts[.left]! }
    set {
      settings.textFonts[.left] = newValue
      update()
    }
  }

  @IBInspectable var leftTextSize: CGFloat {
    get { return settings.textSizes[.left]! }
    set {
      settings.textSizes[.left] = newValue
      update()
    }
  }

  @IBInspectable var leftTextMargin: CGFloat {
    get { return settings.textMargins[.left]! }
    set {
      settings.textMargins[.left] = newValue
      update()
    }
  }

  @IBInspectable var rightText: String? {
    get { return texts[.right] }
    set { texts[.right] = newValue }
  }

  @IBInspectable var rightTextColor: UIColor {
    get { return settings.textColors[.right]! }
    set {
      settings.textColors[.right] = newValue
      update()
    }
  }

  var rightTextFont: UIFont {
    get { return settings.textFonts[.right]! }
    set {
      settings.textFonts[.right] = newValue
      update()
    }
  }

  @IBInspectable var rightTextSize: CGFloat {
    get { return settings.textSizes[.right]! }
    set {
      settings.textSizes[.right] = newValue
      update()
    }
  }

  @IBInspectable var rightTextMargin: CGFloat {
    get { return settings.textMargins[.right]! }
    set {
      settings.textMargins[.right] = newValue
      update()
    }
  }

  @IBInspectable var topText: String? {
    get { return texts[.top] }
    set { texts[.top] = newValue }
  }

  @IBInspectable var topTextColor: UIColor {
    get { return settings.textColors[.top]! }
    set {
      settings.textColors[.top] = newValue
      update()
    }
  }

  var topTextFont: UIFont {
    get { return settings.textFonts[.top]! }
    set {
      settings.textFonts[.top] = newValue
      update()
    }
  }

  @IBInspectable var topTextSize: CGFloat {
    get { return settings.textSizes[.top]! }
    set {
      settings.textSizes[.top] = newValue
      update()
    }
  }

  @IBInspectable var topTextMargin: CGFloat {
    get { return settings.textMargins[.top]! }
    set {
      settings.textMargins[.top] = newValue
      update()
    }
  }

  @IBInspectable var bottomText: String? {
    get { return texts[.bottom] }
    set { texts[.bottom] = newValue }
  }

  @IBInspectable var bottomTextColor: UIColor {
    get { return settings.textColors[.bottom]! }
    set {
      settings.textColors[.bottom] = newValue
      update()
    }
  }

  var bottomTextFont: UIFont {
    get { return settings.textFonts[.bottom]! }
    set {
      settings.textFonts[.bottom] = newValue
      update()
    }
  }

  @IBInspectable var bottomTextSize: CGFloat {
    get { return settings.textSizes[.bottom]! }
    set {
      settings.textSizes[.bottom] = newValue
      update()
    }
  }

  @IBInspectable var bottomTextMargin: CGFloat {
    get { return settings.textMargins[.bottom]! }
    set {
      settings.textMargins[.bottom] = newValue
      update()
    }
  }

  private var texts: [RatingViewSettings.TextSide: String] = [:] {
    didSet { update() }
  }

  @IBInspectable var starType: Int {
    get { return settings.starType.rawValue }
    set {
      settings.starType = RatingViewSettings.StarType(rawValue: newValue) ?? RatingViewSettings.Default.starType
      settings.starPoints = RatingViewSettings.Default.starPoints[settings.starType]!
      update()
    }
  }

  @IBInspectable var starsCount: Int {
    get { return settings.starsCount }
    set {
      settings.starsCount = newValue
      update()
    }
  }

  @IBInspectable var starSize: CGFloat {
    get { return settings.starSize }
    set {
      settings.starSize = newValue
      update()
    }
  }

  @IBInspectable var starMargin: CGFloat {
    get { return settings.starMargin }
    set {
      settings.starMargin = newValue
      update()
    }
  }

  @IBInspectable var filledColor: UIColor {
    get { return settings.filledColor }
    set {
      settings.filledColor = newValue
      update()
    }
  }

  @IBInspectable var emptyColor: UIColor {
    get { return settings.emptyColor }
    set {
      settings.emptyColor = newValue
      update()
    }
  }

  @IBInspectable var emptyBorderColor: UIColor {
    get { return settings.emptyBorderColor }
    set {
      settings.emptyBorderColor = newValue
      update()
    }
  }

  @IBInspectable var borderWidth: CGFloat {
    get { return settings.borderWidth }
    set {
      settings.borderWidth = newValue
      update()
    }
  }

  @IBInspectable var filledBorderColor: UIColor {
    get { return settings.filledBorderColor }
    set {
      settings.filledBorderColor = newValue
      update()
    }
  }

  @IBInspectable var fillMode: Int {
    get { return settings.fillMode.rawValue }
    set {
      settings.fillMode = RatingViewSettings.FillMode(rawValue: newValue) ?? RatingViewSettings.Default.fillMode
      update()
    }
  }

  @IBInspectable var minTouchRating: CGFloat {
    get { return settings.minTouchRating }
    set {
      settings.minTouchRating = newValue
      update()
    }
  }

  @IBInspectable var filledImage: UIImage? {
    get { return settings.filledImage }
    set {
      settings.filledImage = newValue
      update()
    }
  }

  @IBInspectable var emptyImage: UIImage? {
    get { return settings.emptyImage }
    set {
      settings.emptyImage = newValue
      update()
    }
  }

  @IBOutlet weak var delegate: RatingViewDelegate?

  var settings = RatingViewSettings() {
    didSet {
      update()
    }
  }

  private let isRightToLeft = UIApplication.shared.userInterfaceLayoutDirection == .rightToLeft

  public override func awakeFromNib() {
    super.awakeFromNib()
    setup()
    update()
  }

  public convenience init() {
    self.init(frame: CGRect())
  }

  public override init(frame: CGRect) {
    super.init(frame: frame)
    setup()
    update()
  }

  public required init?(coder aDecoder: NSCoder) {
    super.init(coder: aDecoder)
    setup()
    update()
  }

  private func setup() {
    layer.backgroundColor = UIColor.clear.cgColor
    isOpaque = true
  }

  private var viewSize = CGSize()

  private var scheduledUpdate: DispatchWorkItem?

  func updateImpl() {
    let layers = createLayers()
    layer.sublayers = layers

    viewSize = calculateSizeToFitLayers(layers)
    invalidateIntrinsicContentSize()
    frame.size = intrinsicContentSize
  }

  func update() {
    scheduledUpdate?.cancel()
    scheduledUpdate = DispatchWorkItem { [weak self] in self?.updateImpl() }
    DispatchQueue.main.async(execute: scheduledUpdate!)
  }

  override var intrinsicContentSize: CGSize {
    return viewSize
  }

  private func createLayers() -> [CALayer] {
    return combineLayers(starLayers: createStarLayers(), textLayers: createTextLayers())
  }

  private func combineLayers(starLayers: [CALayer], textLayers: [RatingViewSettings.TextSide: CALayer]) -> [CALayer] {
    var layers = starLayers
    var starsWidth: CGFloat = 0
    layers.forEach { starsWidth = max(starsWidth, $0.position.x + $0.bounds.width) }

    if let topTextLayer = textLayers[.top] {
      topTextLayer.position = CGPoint()
      var topTextLayerSize = topTextLayer.bounds.size
      topTextLayerSize.width = max(topTextLayerSize.width, starsWidth)
      topTextLayer.bounds.size = topTextLayerSize
      let y = topTextLayer.position.y + topTextLayer.bounds.height + topTextMargin
      layers.forEach { $0.position.y += y }
    }

    if let bottomTextLayer = textLayers[.bottom] {
      bottomTextLayer.position = CGPoint()
      var bottomTextLayerSize = bottomTextLayer.bounds.size
      bottomTextLayerSize.width = max(bottomTextLayerSize.width, starsWidth)
      bottomTextLayer.bounds.size = bottomTextLayerSize
      let star = layers.first!
      bottomTextLayer.position.y = star.position.y + star.bounds.height + bottomTextMargin
    }

    let leftTextLayer: CALayer?
    let leftMargin: CGFloat
    let rightTextLayer: CALayer?
    let rightMargin: CGFloat

    if isRightToLeft {
      leftTextLayer = textLayers[.right]
      leftMargin = rightTextMargin
      rightTextLayer = textLayers[.left]
      rightMargin = leftTextMargin
    } else {
      leftTextLayer = textLayers[.left]
      leftMargin = leftTextMargin
      rightTextLayer = textLayers[.right]
      rightMargin = rightTextMargin
    }

    if let leftTextLayer = leftTextLayer {
      leftTextLayer.position = CGPoint()
      let star = layers.first!
      leftTextLayer.position.y = star.position.y + (star.bounds.height - leftTextLayer.bounds.height) / 2
      let x = leftTextLayer.position.x + leftTextLayer.bounds.width + leftMargin
      layers.forEach { $0.position.x += x }
      textLayers[.top]?.position.x += x
      textLayers[.bottom]?.position.x += x
    }

    if let rightTextLayer = rightTextLayer {
      rightTextLayer.position = CGPoint()
      let star = layers.last!
      rightTextLayer.position = CGPoint(x: star.position.x + star.bounds.width + rightMargin,
                                        y: star.position.y + (star.bounds.height - rightTextLayer.bounds.height) / 2)
    }

    layers.append(contentsOf: textLayers.values)

    return layers
  }

  private func createStarLayers() -> [CALayer] {
    var ratingRemainder = value
    var starLayers: [CALayer] = (0 ..< settings.starsCount).map { _ in
      let fillLevel = starFillLevel(ratingRemainder: ratingRemainder)
      let layer = createCompositeStarLayer(fillLevel: fillLevel)
      ratingRemainder -= 1
      return layer
    }
    if isRightToLeft {
      starLayers.reverse()
    }
    positionStarLayers(starLayers)
    return starLayers
  }

  private func positionStarLayers(_ layers: [CALayer]) {
    var positionX: CGFloat = 0
    layers.forEach {
      $0.position.x = positionX
      positionX += $0.bounds.width + settings.starMargin
    }
  }

  private func createTextLayers() -> [RatingViewSettings.TextSide: CALayer] {
    var layers: [RatingViewSettings.TextSide: CALayer] = [:]
    for (side, text) in texts {
      let font = settings.textFonts[side]!.withSize(settings.textSizes[side]!)
      let size = NSString(string: text).size(withAttributes: [NSAttributedStringKey.font: font])

      let layer = CATextLayer()
      layer.bounds = CGRect(origin: CGPoint(),
                            size: CGSize(width: ceil(size.width), height: ceil(size.height)))
      layer.anchorPoint = CGPoint()
      layer.string = text
      layer.font = CGFont(font.fontName as CFString)
      layer.fontSize = font.pointSize
      layer.foregroundColor = settings.textColors[side]!.cgColor
      layer.contentsScale = UIScreen.main.scale

      layers[side] = layer
    }
    return layers
  }

  private func starFillLevel(ratingRemainder: CGFloat) -> CGFloat {
    guard ratingRemainder < 1 else { return 1 }
    guard ratingRemainder > 0 else { return 0 }

    switch settings.fillMode {
    case .full: return round(ratingRemainder)
    case .half: return round(ratingRemainder * 2) / 2
    case .precise: return ratingRemainder
    }
  }

  private func calculateSizeToFitLayers(_ layers: [CALayer]) -> CGSize {
    var size = CGSize()
    layers.forEach { layer in
      size.width = max(size.width, layer.frame.maxX)
      size.height = max(size.height, layer.frame.maxY)
    }
    return size
  }

  private func createCompositeStarLayer(fillLevel: CGFloat) -> CALayer {
    guard fillLevel > 0 && fillLevel < 1 else { return createStarLayer(fillLevel >= 1) }
    return createPartialStar(fillLevel: fillLevel)
  }

  func createPartialStar(fillLevel: CGFloat) -> CALayer {
    let filledStar = createStarLayer(true)
    let emptyStar = createStarLayer(false)

    let parentLayer = CALayer()
    parentLayer.contentsScale = UIScreen.main.scale
    parentLayer.bounds = filledStar.bounds
    parentLayer.anchorPoint = CGPoint()
    parentLayer.addSublayer(emptyStar)
    parentLayer.addSublayer(filledStar)

    if isRightToLeft {
      let rotation = CATransform3DMakeRotation(CGFloat.pi, 0, 1, 0)
      filledStar.transform = CATransform3DTranslate(rotation, -filledStar.bounds.size.width, 0, 0)
    }

    filledStar.bounds.size.width *= fillLevel

    return parentLayer
  }

  private func createStarLayer(_ isFilled: Bool) -> CALayer {
    let size = settings.starSize
    let containerLayer = createContainerLayer(size)
    if let image = isFilled ? settings.filledImage : settings.emptyImage {
      let imageLayer = createContainerLayer(size)
      imageLayer.frame = containerLayer.bounds
      imageLayer.contents = image.cgImage
      imageLayer.contentsGravity = kCAGravityResizeAspect
      if image.renderingMode == .alwaysTemplate {
        containerLayer.mask = imageLayer
        containerLayer.backgroundColor = (isFilled ? settings.filledColor : settings.emptyColor).cgColor
      } else {
        containerLayer.addSublayer(imageLayer)
      }
    } else {
      let fillColor = isFilled ? settings.filledColor : settings.emptyColor
      let strokeColor = isFilled ? settings.filledBorderColor : settings.emptyBorderColor
      let lineWidth = settings.borderWidth

      let path = createStarPath(settings.starPoints, size: size, lineWidth: lineWidth)

      let shapeLayer = createShapeLayer(path.cgPath,
                                        lineWidth: lineWidth,
                                        fillColor: fillColor,
                                        strokeColor: strokeColor,
                                        size: size)

      containerLayer.addSublayer(shapeLayer)
    }
    return containerLayer
  }

  private func createContainerLayer(_ size: CGFloat) -> CALayer {
    let layer = CALayer()
    layer.contentsScale = UIScreen.main.scale
    layer.anchorPoint = CGPoint()
    layer.masksToBounds = true
    layer.bounds.size = CGSize(width: size, height: size)
    layer.isOpaque = true
    return layer
  }

  private func createShapeLayer(_ path: CGPath, lineWidth: CGFloat, fillColor: UIColor, strokeColor: UIColor, size: CGFloat) -> CALayer {
    let layer = CAShapeLayer()
    layer.anchorPoint = CGPoint()
    layer.contentsScale = UIScreen.main.scale
    layer.strokeColor = strokeColor.cgColor
    layer.fillRule = kCAFillRuleEvenOdd
    layer.fillColor = fillColor.cgColor
    layer.lineWidth = lineWidth
    layer.lineJoin = kCALineJoinRound
    layer.lineCap = kCALineCapRound
    layer.bounds.size = CGSize(width: size, height: size)
    layer.masksToBounds = true
    layer.path = path
    layer.isOpaque = true

    return layer
  }

  private func createStarPath(_ starPoints: [CGPoint], size: CGFloat, lineWidth: CGFloat) -> UIBezierPath {
    let sizeWithoutLineWidth = size - lineWidth * 2

    let factor = sizeWithoutLineWidth / RatingViewSettings.Default.starPointsBoxSize
    let points = starPoints.map { CGPoint(x: $0.x * factor + lineWidth,
                                          y: $0.y * factor + lineWidth) }

    let path: UIBezierPath
    switch settings.starType {
    case .regular:
      path = UIBezierPath()
    case .boxed:
      path = UIBezierPath(roundedRect: CGRect(origin: CGPoint(), size: CGSize(width: size, height: size)), cornerRadius: size / 4)
    }
    path.move(to: points.first!)
    points[1 ..< points.count].forEach { path.addLine(to: $0) }
    path.close()

    return path
  }

  override func prepareForInterfaceBuilder() {
    super.prepareForInterfaceBuilder()
    updateImpl()
  }

  private func touchLocationFromBeginningOfRating(_ touches: Set<UITouch>) -> CGFloat? {
    guard let touch = touches.first else { return nil }
    var location = touch.location(in: self).x
    let starLayers = layer.sublayers?.filter { !($0 is CATextLayer) }
    var minX = bounds.width
    var maxX: CGFloat = 0
    starLayers?.forEach {
      maxX = max(maxX, $0.position.x + $0.bounds.width)
      minX = min(minX, $0.position.x)
    }
    assert(minX <= maxX)
    if isRightToLeft {
      location = maxX - location
    } else {
      location = location - minX
    }
    return location
  }

  override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
    super.touchesBegan(touches, with: event)
    guard let location = touchLocationFromBeginningOfRating(touches) else { return }
    onDidTouch(location)
  }

  override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
    super.touchesMoved(touches, with: event)
    guard let location = touchLocationFromBeginningOfRating(touches) else { return }
    onDidTouch(location)
  }

  override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
    super.touchesEnded(touches, with: event)
    delegate?.didFinishTouchingRatingView(self)
  }

  private func onDidTouch(_ location: CGFloat) {
    guard let delegate = delegate else { return }
    let prevRating = value

    let starsCount = CGFloat(settings.starsCount)
    let startsLength = settings.starSize * starsCount + (settings.starMargin * (starsCount - 1))
    let percent = location / startsLength
    let preciseRating = percent * starsCount
    let fullStarts = floor(preciseRating)
    let fractionRating = starFillLevel(ratingRemainder: preciseRating - fullStarts)
    value = max(minTouchRating, fullStarts + fractionRating)

    if prevRating != value {
      delegate.didTouchRatingView(self)
    }
  }
}
