extension UIView {
  
  func applyGradient(isVertical: Bool, colorArray: [UIColor]) {
    layer.sublayers?.filter({ $0 is CAGradientLayer }).forEach({ $0.removeFromSuperlayer() })
    
    let gradientLayer = CAGradientLayer()
    gradientLayer.colors = colorArray.map({ $0.cgColor })
    if isVertical {
      //top to bottom
      gradientLayer.locations = [0.0, 1.0]
    } else {
      //left to right
      gradientLayer.startPoint = CGPoint(x: 0.0, y: 0.5)
      gradientLayer.endPoint = CGPoint(x: 1.0, y: 0.5)
    }
    
    backgroundColor = .clear
    gradientLayer.frame = bounds
    layer.insertSublayer(gradientLayer, at: 0)
  }
  
  
}

func gradientColor(yourView:UIView, startColor: UIColor, endColor: UIColor, colorAngle: CGFloat){
    
    let gradientLayer = CAGradientLayer()
    gradientLayer.colors = [startColor.cgColor, endColor.cgColor]
    gradientLayer.locations = [0.0, 1.0]
    let (start, end) = gradientPointsForAngle(colorAngle)
    gradientLayer.startPoint = start
    gradientLayer.endPoint = end
    gradientLayer.frame = yourView.bounds
    
    yourView.layer.insertSublayer(gradientLayer, at: 0)
    yourView.layer.masksToBounds = true
}

func gradientPointsForAngle(_ angle: CGFloat) -> (CGPoint, CGPoint) {
    
    let end = pointForAngle(angle)
    let start = oppositePoint(end)
    let p0 = transformToGradientSpace(start)
    let p1 = transformToGradientSpace(end)
    return (p0, p1)
}

func pointForAngle(_ angle: CGFloat) -> CGPoint {
    let radians = angle * .pi / 180.0
    var x = cos(radians)
    var y = sin(radians)
    
    if (abs(x) > abs(y)) {
        x = x > 0 ? 1 : -1
        y = x * tan(radians)
    } else {
        y = y > 0 ? 1 : -1
        x = y / tan(radians)
    }
    return CGPoint(x: x, y: y)
}

func oppositePoint(_ point: CGPoint) -> CGPoint {
    return CGPoint(x: -point.x, y: -point.y)
}

private func transformToGradientSpace(_ point: CGPoint) -> CGPoint {
    return CGPoint(x: (point.x + 1) * 0.5, y: 1.0 - (point.y + 1) * 0.5)
}
