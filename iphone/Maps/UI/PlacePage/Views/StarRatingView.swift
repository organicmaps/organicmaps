fileprivate class StarView: UIView {
  private static let points: [CGPoint] = [
    CGPoint(x: 49.5, y: 0.0),
    CGPoint(x: 60.5, y: 35.0),
    CGPoint(x: 99.0, y: 35.0),
    CGPoint(x: 67.5, y: 58.0),
    CGPoint(x: 78.5, y: 92.0),
    CGPoint(x: 49.5, y: 71.0),
    CGPoint(x: 20.5, y: 92.0),
    CGPoint(x: 31.5, y: 58.0),
    CGPoint(x: 0.0, y: 35.0),
    CGPoint(x: 38.5, y: 35.0),
  ]

  private var path: UIBezierPath = {
    let path = UIBezierPath()
    path.move(to: StarView.points[0])
    StarView.points.suffix(from: 1).forEach {
      path.addLine(to: $0)
    }
    path.close()
    return path
  } ()

  private var starLayer: CAShapeLayer {
    layer as! CAShapeLayer
  }

  var color: UIColor = UIColor.orange {
    didSet {
      starLayer.fillColor = color.cgColor
    }
  }

  override class var layerClass: AnyClass {
    CAShapeLayer.self
  }

  override init(frame: CGRect) {
    super.init(frame: frame)
    commonInit()
  }

  required init?(coder: NSCoder) {
    super.init(coder: coder)
    commonInit()
  }

  private func commonInit() {
    starLayer.fillRule = .evenOdd
    starLayer.fillColor = color.cgColor
  }

  override func layoutSubviews() {
    super.layoutSubviews()

    let offset: CGFloat = 0.05
    let sx = width / 100 * CGFloat(1.0 - offset * 2)
    let sy = height / 100 * CGFloat(1.0 - offset * 2)

    guard let newPath = path.copy() as? UIBezierPath else { return }
    let scale = CGAffineTransform(scaleX: sx, y: sy)
    let translate = CGAffineTransform(translationX: width * offset, y: height * offset)
    newPath.apply(scale.concatenating(translate))
    let boxPath = UIBezierPath(rect: bounds)
    boxPath.append(newPath)
    boxPath.usesEvenOddFillRule = true
    starLayer.path = boxPath.cgPath
  }
}

class StarRatingView: UIView {
  private var starViews: [StarView] = []

  var rating: Int = 3
  var activeColor: UIColor = UIColor.orange
  var inactiveColor: UIColor = UIColor.lightGray

  override init(frame: CGRect) {
    super.init(frame: frame)
    commonInit()
  }

  required init?(coder: NSCoder) {
    super.init(coder: coder)
    commonInit()
  }

  override func layoutSubviews() {
    super.layoutSubviews()

    for i in 0..<starViews.count {
      let sv = starViews[i]
      sv.color = i >= rating ? inactiveColor : activeColor
      sv.frame = CGRect(x: 18 * i, y: 0, width: 15, height: 15)
      sv.layer.cornerRadius = 3
    }
  }

  override var intrinsicContentSize: CGSize {
    return CGSize(width: 18 * 5 - 3, height: 15)
  }

  private func commonInit() {
    for _ in 0..<5 {
      let sv = StarView()
      sv.color = inactiveColor
      sv.clipsToBounds = true
      starViews.append(sv)
      addSubview(sv)
    }
  }
}
