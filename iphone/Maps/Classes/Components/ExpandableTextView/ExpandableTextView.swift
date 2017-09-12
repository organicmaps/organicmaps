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

  private var isCompact = true {
    didSet {
      guard oldValue != isCompact else { return }
      update()
    }
  }

  private func createTextLayer() -> CALayer {
    let size: CGSize

    let container = CALayer()
    container.anchorPoint = CGPoint()
    container.contentsScale = UIScreen.main.scale

    if isCompact {
      size = text.size(width: frame.width, font: textFont, maxNumberOfLines: numberOfCompactLines)
      let fullSize = text.size(width: frame.width, font: textFont, maxNumberOfLines: 0)
      if size.height < fullSize.height {
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

        container.addSublayer(layer)
      }
    } else {
      size = text.size(width: frame.width, font: textFont, maxNumberOfLines: 0)
    }

    let layer = CATextLayer()
    layer.bounds = CGRect(origin: CGPoint(), size: size)
    layer.anchorPoint = CGPoint()
    layer.string = text
    layer.isWrapped = true
    layer.truncationMode = kCATruncationEnd
    layer.font = CGFont(textFont.fontName as CFString)
    layer.fontSize = textFont.pointSize
    layer.foregroundColor = textColor?.cgColor
    layer.contentsScale = UIScreen.main.scale

    container.addSublayer(layer)

    var containerSize = CGSize()
    container.sublayers?.forEach { layer in
      containerSize.width = max(containerSize.width, layer.frame.maxX)
      containerSize.height = max(containerSize.height, layer.frame.maxY)
    }
    container.frame.size = containerSize
    return container
  }

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
    addGestureRecognizer(UITapGestureRecognizer(target: self, action: #selector(onTap)))
  }

  public required init?(coder aDecoder: NSCoder) {
    super.init(coder: aDecoder)
    setup()
    update()
    addGestureRecognizer(UITapGestureRecognizer(target: self, action: #selector(onTap)))
  }

  private func setup() {
    layer.backgroundColor = UIColor.clear.cgColor
    isOpaque = true
  }

  private var viewSize = CGSize()
  func updateImpl() {
    let sublayer = createTextLayer()
    layer.sublayers = [sublayer]

    viewSize = sublayer.bounds.size

    invalidateIntrinsicContentSize()
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

  override func prepareForInterfaceBuilder() {
    super.prepareForInterfaceBuilder()
    updateImpl()
  }

  @objc private func onTap() {
    isCompact = false
  }
}
