import UIKit

class ChartLineView: UIView {
  override class var layerClass: AnyClass { CAShapeLayer.self }

  private var minX = 0
  private var maxX = 0
  private var minY: CGFloat = 0
  private var maxY: CGFloat = 0

  private var highlightedStroke: CAShapeLayer?

  var lineWidth: CGFloat = 1 {
    didSet {
      updateLineColors()
    }
  }

  var chartLine: ChartPresentationLine! {
    didSet {
      guard let chartLine = chartLine else { return }
      maxX = chartLine.values.count - 1
      minY = chartLine.minY
      maxY = chartLine.maxY
      highlightedStroke?.removeFromSuperlayer()
      highlightedStroke = nil
      if chartLine.highlighted != nil, chartLine.type == .line {
        let stroke = CAShapeLayer()
        stroke.actions = ["path": NSNull(), "bounds": NSNull(), "position": NSNull()]
        stroke.fillColor = nil
        stroke.lineJoin = .round
        stroke.lineCap = .round
        shapeLayer.addSublayer(stroke)
        highlightedStroke = stroke
      }
      updateLineColors()
      updateGraph()
    }
  }

  override init(frame: CGRect) {
    super.init(frame: frame)
    transform = CGAffineTransform.identity.scaledBy(x: 1, y: -1)
    isUserInteractionEnabled = false
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError()
  }

  var shapeLayer: CAShapeLayer {
    layer as! CAShapeLayer
  }

  func setViewport(minX: Int, maxX: Int, minY: CGFloat, maxY: CGFloat, animationStyle: ChartAnimation = .none) {
    guard minX < maxX, minY < maxY else { return }
    self.minX = minX
    self.maxX = maxX
    self.minY = minY
    self.maxY = maxY
    updateGraph(animationStyle: animationStyle)
  }

  private func updateLineColors() {
    guard let chartLine else { return }
    let resolvedColor = chartLine.color.resolvedColor(with: traitCollection).cgColor
    switch chartLine.type {
    case .line:
      shapeLayer.strokeColor = highlightedStroke == nil ? resolvedColor : UIColor.secondaryLabel.resolvedColor(with: traitCollection).cgColor
      shapeLayer.lineWidth = highlightedStroke == nil ? lineWidth : 1
      highlightedStroke?.strokeColor = resolvedColor
      highlightedStroke?.lineWidth = lineWidth
      shapeLayer.fillColor = UIColor.clear.cgColor
      shapeLayer.lineJoin = .round
    case .lineArea:
      shapeLayer.strokeColor = UIColor.clear.cgColor
      shapeLayer.fillColor = resolvedColor
    }
  }

  private func updateGraph(animationStyle: ChartAnimation = .none) {
    // A flat profile gets its Y span from the viewport.
    guard let chartLine, maxX > minX, maxY > minY else { return }

    let xScale = bounds.width / CGFloat(maxX - minX)
    let xTranslate = -bounds.width * CGFloat(minX) / CGFloat(maxX - minX)
    let yScale = (bounds.height - 1) / CGFloat(maxY - minY)
    let yTranslate = (bounds.height - 1) * CGFloat(chartLine.minY - minY) / CGFloat(maxY - minY) + 0.5
    let scale = CGAffineTransform.identity.scaledBy(x: xScale, y: yScale)
    let translate = CGAffineTransform.identity.translatedBy(x: xTranslate, y: yTranslate)
    let transform = scale.concatenating(translate)
    let realPath = CGMutablePath()
    let section = chartLine.highlighted
    realPath.addPath(chartLine.path, transform: transform)

    if let section, let highlightedStroke {
      let stroke = CGMutablePath()
      stroke.addPath(section.path, transform: transform)
      let entryX = section.entry.x * xScale + xTranslate
      let delta = stroke.currentPoint.x - entryX
      let sign: CGFloat = delta < 0 ? -1 : 1
      let length = min(18, abs(delta))
      let tipX = entryX + sign * length
      let head = min(4, length / 2)
      let y = bounds.height - 8
      stroke.move(to: CGPoint(x: entryX, y: y))
      stroke.addLine(to: CGPoint(x: tipX, y: y))
      stroke.move(to: CGPoint(x: tipX - sign * head, y: y - head))
      stroke.addLine(to: CGPoint(x: tipX, y: y))
      stroke.addLine(to: CGPoint(x: tipX - sign * head, y: y + head))
      highlightedStroke.frame = bounds
      highlightedStroke.path = stroke
    }

    if chartLine.type == .lineArea, !realPath.isEmpty {
      // Close area fills at the viewport bottom, regardless of sea level.
      let startX = (section?.entry.x ?? chartLine.pathStartX) * xScale + xTranslate
      realPath.addLine(to: CGPoint(x: realPath.currentPoint.x, y: 0))
      realPath.addLine(to: CGPoint(x: startX, y: 0))
      realPath.closeSubpath()
    }

    if animationStyle != .none {
      let timingFunction = CAMediaTimingFunction(name: animationStyle == .interactive ? .linear : .easeInEaseOut)
      if shapeLayer.animationKeys()?.contains("path") ?? false,
         let presentation = shapeLayer.presentation(),
         let path = presentation.path {
        shapeLayer.removeAnimation(forKey: "path")
        shapeLayer.path = path
      }

      let animation = CABasicAnimation(keyPath: "path")
      animation.duration = animationStyle.rawValue
      animation.fromValue = shapeLayer.path
      animation.timingFunction = timingFunction
      layer.add(animation, forKey: "path")
    }

    shapeLayer.path = realPath
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    updateGraph()
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    guard traitCollection.hasDifferentColorAppearance(comparedTo: previousTraitCollection) else { return }
    updateLineColors()
  }
}
