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

class ChartPointIntersectionView: UIView {
  fileprivate var intersectionView = CircleView()

  var color: UIColor = UIColor(red: 0.14, green: 0.61, blue: 0.95, alpha: 1) {
    didSet {
      intersectionView.color = color
      backgroundColor = color.withAlphaComponent(0.5)
    }
  }

  override init(frame: CGRect) {
    super.init(frame: frame)
    backgroundColor = color.withAlphaComponent(0.5)
    transform = CGAffineTransform.identity.scaledBy(x: 1, y: -1)

    intersectionView.color = color
    intersectionView.frame = CGRect(x: 0, y: 0, width: 24, height: 24)
    intersectionView.center = CGPoint(x: bounds.midX, y: bounds.midY)
    addSubview(intersectionView)
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError()
  }

  func updatePoint(_ point: ChartLineInfo) {
    intersectionView.center = CGPoint(x: bounds.midX, y: point.point.y)
    color = point.color
  }
}
