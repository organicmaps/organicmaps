import UIKit

class ChartMyPositionView: UIView {
  override class var layerClass: AnyClass { CAShapeLayer.self }
  var shapeLayer: CAShapeLayer { layer as! CAShapeLayer }
  var pinY: CGFloat = 0 {
    didSet {
      updatePin()
    }
  }

  fileprivate let pinView = MyPositionPinView(frame: CGRect(x: 0, y: 0, width: 12, height: 16))

  override init(frame: CGRect) {
    super.init(frame: frame)

    shapeLayer.lineDashPattern = [3, 2]
    shapeLayer.lineWidth = 2
    shapeLayer.strokeColor = UIColor(red: 0.142, green: 0.614, blue: 0.95, alpha: 0.3).cgColor
    addSubview(pinView)
    transform = CGAffineTransform.identity.scaledBy(x: 1, y: -1)
    pinView.transform = CGAffineTransform.identity.scaledBy(x: 1, y: -1)
  }
  
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  override func layoutSubviews() {
    super.layoutSubviews()
    let path = UIBezierPath()
    path.move(to: CGPoint(x: bounds.width / 2, y: 0))
    path.addLine(to: CGPoint(x: bounds.width / 2, y: bounds.height))
    shapeLayer.path = path.cgPath

    updatePin()
  }

  private func updatePin() {
    pinView.center = CGPoint(x: bounds.midX, y: pinY + pinView.bounds.height / 2)
  }
}

fileprivate class MyPositionPinView: UIView {
  override class var layerClass: AnyClass { CAShapeLayer.self }
  var shapeLayer: CAShapeLayer { layer as! CAShapeLayer }

  var path: UIBezierPath = {
    let p = UIBezierPath()
    p.addArc(withCenter: CGPoint(x: 6, y: 6),
             radius: 6,
             startAngle: -CGFloat.pi / 2,
             endAngle: atan(3.5 / 4.8733971724),
             clockwise: true)
    p.addLine(to: CGPoint(x: 6 + 0.75, y: 15.6614378))
    p.addArc(withCenter: CGPoint(x: 6, y: 15),
             radius: 1,
             startAngle: atan(0.6614378 / 0.75),
             endAngle: CGFloat.pi - atan(0.6614378 / 0.75),
             clockwise: true)
    p.addLine(to: CGPoint(x: 6 - 4.8733971724, y: 9.5))
    p.addArc(withCenter: CGPoint(x: 6, y: 6),
             radius: 6,
             startAngle: CGFloat.pi - atan(3.5 / 4.8733971724),
             endAngle: -CGFloat.pi / 2,
             clockwise: true)
    p.close()

    p.append(UIBezierPath(ovalIn: CGRect(x: 3, y: 3, width: 6, height: 6)))
    return p
  }()

  override init(frame: CGRect) {
    super.init(frame: frame)

    shapeLayer.lineWidth = 0
    shapeLayer.fillColor = UIColor(red: 0.142, green: 0.614, blue: 0.95, alpha: 0.5).cgColor
    shapeLayer.fillRule = .evenOdd
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func layoutSubviews() {
    super.layoutSubviews()

    let sx = bounds.width / path.bounds.width
    let sy = bounds.height / path.bounds.height
    let p = path.copy() as! UIBezierPath
    p.apply(CGAffineTransform(scaleX: sx, y: sy))
    shapeLayer.path = p.cgPath
  }
}
