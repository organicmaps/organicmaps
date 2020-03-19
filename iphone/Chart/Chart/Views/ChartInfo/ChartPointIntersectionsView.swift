import UIKit

fileprivate class CircleView: UIView {
  override class var layerClass: AnyClass { return CAShapeLayer.self }

  var color: UIColor? {
    didSet {
      shapeLayer.fillColor = color?.withAlphaComponent(0.5).cgColor
      ringLayer.fillColor = UIColor.white.cgColor
      centerLayer.fillColor = color?.cgColor
    }
  }

  var shapeLayer: CAShapeLayer {
    return layer as! CAShapeLayer
  }

  let ringLayer = CAShapeLayer()
  let centerLayer = CAShapeLayer()

  override var frame: CGRect {
    didSet {
      let p = UIBezierPath(ovalIn: bounds)
      shapeLayer.path = p.cgPath
      ringLayer.frame = shapeLayer.bounds.insetBy(dx: shapeLayer.bounds.width / 6, dy: shapeLayer.bounds.height / 6)
      ringLayer.path = UIBezierPath(ovalIn: ringLayer.bounds).cgPath
      centerLayer.frame = shapeLayer.bounds.insetBy(dx: shapeLayer.bounds.width / 3, dy: shapeLayer.bounds.height / 3)
      centerLayer.path = UIBezierPath(ovalIn: centerLayer.bounds).cgPath
    }
  }

  override init(frame: CGRect) {
    super.init(frame: frame)

    shapeLayer.fillColor = color?.withAlphaComponent(0.5).cgColor
    shapeLayer.lineWidth = 4
    shapeLayer.fillRule = .evenOdd
    shapeLayer.addSublayer(ringLayer)
    shapeLayer.addSublayer(centerLayer)
    ringLayer.fillColor = UIColor.white.cgColor
    centerLayer.fillColor = color?.cgColor
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError()
  }
}

class ChartPointIntersectionsView: UIView {
  fileprivate var intersectionViews: [CircleView] = []

  override init(frame: CGRect) {
    super.init(frame: frame)
    backgroundColor = UIColor(red: 0.14, green: 0.61, blue: 0.95, alpha: 0.5)
    transform = CGAffineTransform.identity.scaledBy(x: 1, y: -1)
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError()
  }

  func setPoints(_ points: [ChartLineInfo]) {
    intersectionViews.forEach { $0.removeFromSuperview() }
    intersectionViews.removeAll()

    for point in points {
      let v = CircleView()
      v.color = point.color
      v.frame = CGRect(x: 0, y: 0, width: 24, height: 24)
      v.center = CGPoint(x: bounds.midX, y: point.point.y)
      intersectionViews.append(v)
      addSubview(v)
    }
  }

  func updatePoints(_ points: [ChartLineInfo]) {
    for i in 0..<intersectionViews.count {
      let v = intersectionViews[i]
      let p = points[i]
      v.center = CGPoint(x: bounds.midX, y: p.point.y)
    }
  }
}

