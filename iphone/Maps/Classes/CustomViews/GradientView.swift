@IBDesignable
class GradientView: UIView {
  enum GradientDirection {
    case horizontal
    case vertical
  }

  var gradientDirection: GradientDirection = .vertical {
    didSet {
      let angle = gradientDirection == .horizontal ? -CGFloat.pi / 2 : 0
      gradientLayer.transform = CATransform3DMakeRotation(angle, 0, 0, 1)
    }
  }

  @IBInspectable
  var startColor: UIColor = .clear {
    didSet {
      updateColors()
    }
  }

  @IBInspectable
  var endColor: UIColor = .lightGray {
    didSet {
      updateColors()
    }
  }

  private func updateColors() {
    gradientLayer.colors = [startColor.cgColor, endColor.cgColor]
  }

  override class var layerClass: AnyClass {
    return CAGradientLayer.self
  }

  var gradientLayer: CAGradientLayer {
    return layer as! CAGradientLayer
  }

  override init(frame: CGRect) {
    super.init(frame: frame)
    updateColors()
  }

  required init?(coder aDecoder: NSCoder) {
    super.init(coder: aDecoder)
    updateColors()
  }
}
