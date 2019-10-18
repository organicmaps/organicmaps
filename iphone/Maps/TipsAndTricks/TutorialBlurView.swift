@objc(MWMTutorialBlurView)
class TutorialBlurView: UIVisualEffectView {
  var targetView: UIView?
  private var maskPath: UIBezierPath?
  private var maskLayer = CAShapeLayer()
  private let layoutView = UIView(frame: CGRect(x: -100, y: -100, width: 0, height: 0))

  private func setup() {
    maskLayer.fillRule = CAShapeLayerFillRule.evenOdd
    layer.mask = maskLayer
    layoutView.translatesAutoresizingMaskIntoConstraints = false
    layoutView.isUserInteractionEnabled = false
    contentView.addSubview(layoutView)
    effect = nil
  }

  override init(effect: UIVisualEffect?) {
    super.init(effect: effect)
    setup()
  }

  required init?(coder aDecoder: NSCoder) {
    super.init(coder: aDecoder)
    setup()
  }

  override func point(inside point: CGPoint, with event: UIEvent?) -> Bool {
    guard let pointInView = targetView?.bounds.contains(convert(point, to: targetView)) else {
      return super.point(inside: point, with: event)
    }
    return !pointInView
  }

  override func didMoveToSuperview() {
    super.didMoveToSuperview()
    if superview != nil {
      targetView?.centerXAnchor.constraint(equalTo: layoutView.centerXAnchor).isActive = true
      targetView?.centerYAnchor.constraint(equalTo: layoutView.centerYAnchor).isActive = true
      guard #available(iOS 11.0, *) else {
        DispatchQueue.main.async {
          self.setNeedsLayout()
        }
        return
      }
    }
  }

  override func layoutSubviews() {
    super.layoutSubviews()

    let targetCenter = layoutView.center
    let r: CGFloat = 40
    let targetRect = CGRect(x: targetCenter.x - r, y: targetCenter.y - r, width: r * 2, height: r * 2)
    maskPath = UIBezierPath(rect: bounds)
    maskPath!.append(UIBezierPath(ovalIn: targetRect))
    maskPath!.usesEvenOddFillRule = true
    maskLayer.path = maskPath!.cgPath

    let pulsationPath = UIBezierPath(rect: bounds)
    pulsationPath.append(UIBezierPath(ovalIn: targetRect.insetBy(dx: -10, dy: -10)))
    pulsationPath.usesEvenOddFillRule = true
    addPulsation(pulsationPath)
  }

  func animateSizeChange(_ duration: TimeInterval) {
    layer.mask = nil
    DispatchQueue.main.asyncAfter(deadline: .now() + duration) {
      self.layer.mask = self.maskLayer
      self.setNeedsLayout()
    }
  }

  func animateFadeOut(_ duration: TimeInterval, completion: @escaping () -> Void) {
    UIView.animate(withDuration: duration, animations: {
      self.effect = nil
      self.contentView.alpha = 0
    }) { _ in
      self.contentView.backgroundColor = .clear
      completion()
    }
  }

  func animateAppearance(_ duration: TimeInterval) {
    contentView.alpha = 0
    UIView.animate(withDuration: duration) {
      self.contentView.alpha = 1
      self.effect = UIBlurEffect(style: UIColor.isNightMode() ? .light : .dark)
    }
  }

  private func addPulsation(_ path: UIBezierPath) {
    let animation = CABasicAnimation(keyPath: "path")
    animation.duration = kDefaultAnimationDuration
    animation.fromValue = maskLayer.path
    animation.toValue = path.cgPath
    animation.autoreverses = true
    animation.timingFunction = CAMediaTimingFunction(name: CAMediaTimingFunctionName.easeInEaseOut)
    animation.repeatCount = 2

    let animationGroup = CAAnimationGroup()
    animationGroup.duration = 3
    animationGroup.repeatCount = Float(Int.max)
    animationGroup.animations = [animation]

    maskLayer.add(animationGroup, forKey: "path")
  }
}
