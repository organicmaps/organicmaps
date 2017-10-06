import UIKit

@IBDesignable final class RatingSummaryView: UIView {
  @IBInspectable var value: String = RatingSummaryViewSettings.Default.value {
    didSet {
      guard oldValue != value else { return }
      update()
    }
  }

  var type = MWMRatingSummaryViewValueType.noValue {
    didSet {
      guard oldValue != type else { return }
      update()
    }
  }

  @IBInspectable var topOffset: CGFloat {
    get { return settings.topOffset }
    set {
      settings.topOffset = newValue
      update()
    }
  }

  @IBInspectable var bottomOffset: CGFloat {
    get { return settings.bottomOffset }
    set {
      settings.bottomOffset = newValue
      update()
    }
  }

  @IBInspectable var leadingImageOffset: CGFloat {
    get { return settings.leadingImageOffset }
    set {
      settings.leadingImageOffset = newValue
      update()
    }
  }

  @IBInspectable var margin: CGFloat {
    get { return settings.margin }
    set {
      settings.margin = newValue
      update()
    }
  }

  @IBInspectable var trailingTextOffset: CGFloat {
    get { return settings.trailingTextOffset }
    set {
      settings.trailingTextOffset = newValue
      update()
    }
  }

  var textFont: UIFont {
    get { return settings.textFont }
    set {
      settings.textFont = newValue
      update()
    }
  }

  @IBInspectable var textSize: CGFloat {
    get { return settings.textSize }
    set {
      settings.textSize = newValue
      update()
    }
  }

  @IBInspectable var backgroundOpacity: CGFloat {
    get { return settings.backgroundOpacity }
    set {
      settings.backgroundOpacity = newValue
      update()
    }
  }

  @IBInspectable var noValueText: String {
    get { return settings.noValueText }
    set {
      settings.noValueText = newValue
      update()
    }
  }

  @IBInspectable var noValueImage: UIImage? {
    get { return settings.images[.noValue] }
    set {
      settings.images[.noValue] = newValue
      update()
    }
  }

  @IBInspectable var noValueColor: UIColor? {
    get { return settings.colors[.noValue] }
    set {
      settings.colors[.noValue] = newValue
      update()
    }
  }

  @IBInspectable var horribleImage: UIImage? {
    get { return settings.images[.horrible] }
    set {
      settings.images[.horrible] = newValue
      update()
    }
  }

  @IBInspectable var horribleColor: UIColor? {
    get { return settings.colors[.horrible] }
    set {
      settings.colors[.horrible] = newValue
      update()
    }
  }

  @IBInspectable var badImage: UIImage? {
    get { return settings.images[.bad] }
    set {
      settings.images[.bad] = newValue
      update()
    }
  }

  @IBInspectable var badColor: UIColor? {
    get { return settings.colors[.bad] }
    set {
      settings.colors[.bad] = newValue
      update()
    }
  }

  @IBInspectable var normalImage: UIImage? {
    get { return settings.images[.normal] }
    set {
      settings.images[.normal] = newValue
      update()
    }
  }

  @IBInspectable var normalColor: UIColor? {
    get { return settings.colors[.normal] }
    set {
      settings.colors[.normal] = newValue
      update()
    }
  }

  @IBInspectable var goodImage: UIImage? {
    get { return settings.images[.good] }
    set {
      settings.images[.good] = newValue
      update()
    }
  }

  @IBInspectable var goodColor: UIColor? {
    get { return settings.colors[.good] }
    set {
      settings.colors[.good] = newValue
      update()
    }
  }

  @IBInspectable var excellentImage: UIImage? {
    get { return settings.images[.excellent] }
    set {
      settings.images[.excellent] = newValue
      update()
    }
  }

  @IBInspectable var excellentColor: UIColor? {
    get { return settings.colors[.excellent] }
    set {
      settings.colors[.excellent] = newValue
      update()
    }
  }

  var settings = RatingSummaryViewSettings() {
    didSet {
      update()
    }
  }

  private var isRightToLeft = UIApplication.shared.userInterfaceLayoutDirection == .rightToLeft

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
  func updateImpl() {
    let sublayer = createLayer()
    layer.sublayers = [sublayer]

    viewSize = sublayer.bounds.size
    invalidateIntrinsicContentSize()
    frame.size = intrinsicContentSize
  }

  @objc func doUpdate() {
    DispatchQueue.main.async {
      self.updateImpl()
    }
  }

  func update() {
    let sel = #selector(doUpdate)
    NSObject.cancelPreviousPerformRequests(withTarget: self, selector: sel, object: nil)
    perform(sel, with: nil, afterDelay: 1 / 120)
  }

  override var intrinsicContentSize: CGSize {
    return viewSize
  }

  private func createLayer() -> CALayer {
    let textLayer = createTextLayer()
    return combineLayers(imageLayer: createImageLayer(textLayer.bounds.height),
                         textLayer: textLayer)
  }

  private func combineLayers(imageLayer: CALayer?, textLayer: CALayer) -> CALayer {
    var x: CGFloat = 0
    let height = textLayer.bounds.height + settings.topOffset + settings.bottomOffset
    let containerLayer = createContainerLayer(height)
    if let imageLayer = imageLayer {
      x += settings.leadingImageOffset
      containerLayer.addSublayer(imageLayer)
      imageLayer.position = CGPoint(x: x, y: settings.topOffset)
      x += imageLayer.bounds.width

      containerLayer.backgroundColor = settings.colors[type]!.withAlphaComponent(settings.backgroundOpacity).cgColor
      containerLayer.cornerRadius = height / 2
    }

    containerLayer.addSublayer(textLayer)
    x += settings.margin
    textLayer.position = CGPoint(x: x, y: settings.topOffset)
    x += textLayer.bounds.width + settings.trailingTextOffset

    containerLayer.bounds.size.width = x

    if isRightToLeft {
      let rotation = CATransform3DMakeRotation(CGFloat.pi, 0, 1, 0)
      containerLayer.transform = CATransform3DTranslate(rotation, -containerLayer.bounds.size.width, 0, 0)
      textLayer.transform = CATransform3DTranslate(rotation, -textLayer.bounds.size.width, 0, 0)
    }
    return containerLayer
  }

  private func createImageLayer(_ size: CGFloat) -> CALayer? {
    guard let image = settings.images[type] else { return nil }
    let imageLayer = createContainerLayer(size)
    imageLayer.contents = image.cgImage
    imageLayer.contentsGravity = kCAGravityResizeAspect

    let containerLayer = createContainerLayer(size)
    if image.renderingMode == .alwaysTemplate {
      containerLayer.mask = imageLayer
      containerLayer.backgroundColor = settings.colors[type]!.cgColor
    } else {
      containerLayer.addSublayer(imageLayer)
    }
    return containerLayer
  }

  private func createTextLayer() -> CALayer {
    let font = textFont.withSize(textSize)
    let size = NSString(string: value).size(withAttributes: [NSAttributedStringKey.font: font])

    let layer = CATextLayer()
    layer.bounds = CGRect(origin: CGPoint(),
                          size: CGSize(width: ceil(size.width), height: ceil(size.height)))
    layer.anchorPoint = CGPoint()
    layer.string = value
    layer.font = CGFont(font.fontName as CFString)
    layer.fontSize = font.pointSize
    layer.foregroundColor = settings.colors[type]!.cgColor
    layer.contentsScale = UIScreen.main.scale

    return layer
  }

  private func createContainerLayer(_ size: CGFloat) -> CALayer {
    let layer = CALayer()
    layer.bounds = CGRect(origin: CGPoint(), size: CGSize(width: size, height: size))
    layer.anchorPoint = CGPoint()
    layer.contentsGravity = kCAGravityResizeAspect
    layer.contentsScale = UIScreen.main.scale
    layer.masksToBounds = true
    layer.isOpaque = true
    return layer
  }

  override func prepareForInterfaceBuilder() {
    super.prepareForInterfaceBuilder()
    updateImpl()
  }
}
