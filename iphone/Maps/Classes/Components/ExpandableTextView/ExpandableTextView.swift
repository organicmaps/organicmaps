import UIKit

@IBDesignable final class ExpandableTextView: UIView {
  @IBInspectable var text: String = "" {
    didSet {
      guard oldValue != text else { return }
      update()
    }
  }

  @IBInspectable var textColor: UIColor? {
    get { return settings.textColor }
    set {
      settings.textColor = newValue ?? settings.textColor
      update()
    }
  }

  @IBInspectable var expandText: String {
    get { return settings.expandText }
    set {
      settings.expandText = newValue
      update()
    }
  }

  @IBInspectable var expandTextColor: UIColor? {
    get { return settings.expandTextColor }
    set {
      settings.expandTextColor = newValue ?? settings.expandTextColor
      update()
    }
  }

  @IBInspectable var numberOfCompactLines: Int {
    get { return settings.numberOfCompactLines }
    set {
      settings.numberOfCompactLines = newValue
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

  var settings = ExpandableTextViewSettings() {
    didSet { update() }
  }

  var onUpdate: (() -> Void)?

  override var frame: CGRect {
    didSet {
      guard frame.size != oldValue.size else { return }
      update()
    }
  }

  override var bounds: CGRect {
    didSet {
      guard bounds.size != oldValue.size else { return }
      update()
    }
  }

  private var isCompact = true {
    didSet {
      guard oldValue != isCompact else { return }
      update()
    }
  }

  private func createTextLayer() {
    self.layer.sublayers?.forEach { $0.removeFromSuperlayer() }

    var truncate = false
    let size: CGSize
    let fullSize = text.size(width: frame.width, font: textFont, maxNumberOfLines: 0)
    if isCompact {
      size = text.size(width: frame.width, font: textFont, maxNumberOfLines: numberOfCompactLines)
      truncate = size.height < fullSize.height
      if truncate {
        let expandSize = expandText.size(width: frame.width, font: textFont, maxNumberOfLines: 1)
        let layer = CATextLayer()
        layer.position = CGPoint(x: 0, y: size.height)
        layer.bounds = CGRect(origin: CGPoint(), size: expandSize)
        layer.anchorPoint = CGPoint()
        layer.string = expandText
        layer.font = CGFont(textFont.fontName as CFString)
        layer.fontSize = textFont.pointSize
        layer.foregroundColor = expandTextColor?.cgColor
        layer.contentsScale = UIScreen.main.scale

        self.layer.addSublayer(layer)
      }
    } else {
      size = fullSize
    }

    let layer = CATextLayer()
    layer.bounds = CGRect(origin: CGPoint(), size: size)
    layer.anchorPoint = CGPoint()
    layer.string = text
    layer.isWrapped = true
    layer.truncationMode = truncate ? kCATruncationEnd : kCATruncationNone
    layer.font = CGFont(textFont.fontName as CFString)
    layer.fontSize = textFont.pointSize
    layer.foregroundColor = textColor?.cgColor
    layer.contentsScale = UIScreen.main.scale

    self.layer.addSublayer(layer)
  }

  public override func awakeFromNib() {
    super.awakeFromNib()
    setup()
  }

  public convenience init() {
    self.init(frame: CGRect())
  }

  public override init(frame: CGRect) {
    super.init(frame: frame)
    setup()
  }

  public required init?(coder aDecoder: NSCoder) {
    super.init(coder: aDecoder)
    setup()
  }

  override func mwm_refreshUI() {
    super.mwm_refreshUI()
    textColor = textColor?.opposite()
    expandTextColor = expandTextColor?.opposite()
  }

  private func setup() {
    layer.backgroundColor = UIColor.clear.cgColor
    isOpaque = true
    gestureRecognizers = nil
    addGestureRecognizer(UITapGestureRecognizer(target: self, action: #selector(onTap)))
    update()
  }

  private var scheduledUpdate: DispatchWorkItem?

  private func updateImpl() {
    createTextLayer()

    invalidateIntrinsicContentSize()
    onUpdate?()
  }

  func update() {
    scheduledUpdate?.cancel()
    scheduledUpdate = DispatchWorkItem { [weak self] in self?.updateImpl() }
    DispatchQueue.main.async(execute: scheduledUpdate!)
  }

  override var intrinsicContentSize: CGSize {
    var viewSize = CGSize()
    layer.sublayers?.forEach { layer in
      viewSize.width = max(viewSize.width, layer.frame.maxX)
      viewSize.height = max(viewSize.height, layer.frame.maxY)
    }
    return viewSize
  }

  override func prepareForInterfaceBuilder() {
    super.prepareForInterfaceBuilder()
    updateImpl()
  }

  @objc private func onTap() {
    isCompact = false
  }
}
