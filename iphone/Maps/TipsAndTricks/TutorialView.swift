protocol ITutorialView: AnyObject {
  var targetView: UIView? { get set }
  func animateSizeChange(_ duration: TimeInterval)
  func animateAppearance(_ duration: TimeInterval)
  func animateFadeOut(_ duration: TimeInterval, completion: @escaping () -> Void)
}

@objc(MWMTutorialView)
class TutorialView: UIView, ITutorialView {
  func animateFadeOut(_ duration: TimeInterval, completion: @escaping () -> Void) {
    
  }

  func animateAppearance(_ duration: TimeInterval) {
    
  }

  var targetView: UIView?
  var maskPath: UIBezierPath?
  var maskLayer: CAShapeLayer!
  let layoutView = UIView()

  override init(frame: CGRect) {
    super.init(frame: frame)
    setup()
  }

  required init?(coder aDecoder: NSCoder) {
    super.init(coder: aDecoder)
    setup()
  }

  private func setup() {
    maskLayer = CAShapeLayer()
    maskLayer.fillRule = kCAFillRuleEvenOdd
    layer.mask = maskLayer
    layoutView.translatesAutoresizingMaskIntoConstraints = false
    layoutView.isUserInteractionEnabled = false
    layoutView.alpha = 0
    addSubview(layoutView)
  }

  override func didMoveToSuperview() {
    super.didMoveToSuperview()
    if (superview != nil) {
      targetView?.centerXAnchor.constraint(equalTo: layoutView.centerXAnchor).isActive = true
      targetView?.centerYAnchor.constraint(equalTo: layoutView.centerYAnchor).isActive = true
//      targetView?.leftAnchor.constraint(equalTo: layoutView.leftAnchor).isActive = true
//      targetView?.leftAnchor.constraint(equalTo: layoutView.leftAnchor).isActive = true
    }
  }

  override func point(inside point: CGPoint, with event: UIEvent?) -> Bool {
    return maskPath?.contains(point) ?? super.point(inside: point, with: event)
  }

  private var animate = false
  private var ad: TimeInterval = 0

  func animateSizeChange(_ duration: TimeInterval) {
    animate = true
    ad = duration
  }

  override func layoutSubviews() {
    super.layoutSubviews()
//    guard let point = targetView?.center else { return }
//    let targetCenter = convert(point, from: targetView?.superview)
    let targetCenter = layoutView.center
    let r: CGFloat = 40
    let targetRect = CGRect(x: targetCenter.x - r, y: targetCenter.y - r, width: r * 2, height: r * 2)
    maskPath = UIBezierPath(rect: bounds)
    maskPath!.append(UIBezierPath(ovalIn: targetRect))
    maskPath!.usesEvenOddFillRule = true

    if (animate) {
      animatePathChange(for: maskLayer, fromPath: maskLayer.path)
      animate = false
    }

    maskLayer.path = maskPath!.cgPath
  }

  private func animatePathChange(for layer: CAShapeLayer, fromPath: CGPath?) {
    let animation = CABasicAnimation(keyPath: "path")
    animation.duration = ad
    animation.fromValue = fromPath
    animation.timingFunction = CAMediaTimingFunction(name: kCAMediaTimingFunctionEaseInEaseOut)
    layer.add(animation, forKey: "path")
  }
}
