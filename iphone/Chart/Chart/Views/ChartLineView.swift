import UIKit

class ChartLineView: UIView {
  override class var layerClass: AnyClass { CAShapeLayer.self }

  private var minX = 0
  private var maxX = 0
  private var minY: CGFloat = 0
  private var maxY: CGFloat = 0

  private var highlightedStroke: CAShapeLayer?
  private var directionArrow: CAShapeLayer?

  var chartLine: ChartPresentationLine! {
    didSet {
      guard let chartLine else { return }
      let lineWidth: CGFloat = 3
      highlightedStroke?.removeFromSuperlayer()
      highlightedStroke = nil
      directionArrow?.removeFromSuperlayer()
      directionArrow = nil
      if chartLine.highlighted != nil, chartLine.type == .line {
        let stroke = CAShapeLayer()
        stroke.actions = ["path": NSNull(), "bounds": NSNull(), "position": NSNull()]
        stroke.fillColor = nil
        stroke.lineWidth = lineWidth
        stroke.lineJoin = .round
        stroke.lineCap = .round
        shapeLayer.addSublayer(stroke)
        highlightedStroke = stroke
        let arrow = CAShapeLayer()
        arrow.actions = stroke.actions
        shapeLayer.addSublayer(arrow)
        directionArrow = arrow
      }
      shapeLayer.lineWidth = highlightedStroke == nil ? lineWidth : 1
      updateLineColors()
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
      highlightedStroke?.strokeColor = resolvedColor
      directionArrow?.fillColor = resolvedColor
      shapeLayer.fillColor = UIColor.clear.cgColor
      shapeLayer.lineJoin = .round
    case .lineArea:
      shapeLayer.strokeColor = UIColor.clear.cgColor
      shapeLayer.fillColor = resolvedColor
    }
  }

  func updateGraph(animationStyle: ChartAnimation = .none) {
    // Layout can run before the chart supplies its viewport.
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

    if let section, let highlightedStroke, let directionArrow {
      let stroke = CGMutablePath()
      stroke.addPath(section.path, transform: transform)
      let entryX = section.entry.x * xScale + xTranslate
      let delta = stroke.currentPoint.x - entryX
      let sign: CGFloat = delta < 0 ? -1 : 1
      let length = min(18, abs(delta))
      let tipX = entryX + sign * length
      let head = min(4, length / 2)
      let y = bounds.height - 8
      highlightedStroke.frame = bounds
      highlightedStroke.path = stroke

      let width = highlightedStroke.lineWidth
      let arrow = CGMutablePath()
      // The shaft meets the selector with a flat edge; the head keeps rounded ends.
      let shaft = CGMutablePath()
      shaft.move(to: CGPoint(x: entryX, y: y))
      shaft.addLine(to: CGPoint(x: tipX, y: y))
      arrow.addPath(shaft.copy(strokingWithWidth: width, lineCap: .butt, lineJoin: .round, miterLimit: 10))
      let arrowhead = CGMutablePath()
      arrowhead.move(to: CGPoint(x: tipX - sign * head, y: y - head))
      arrowhead.addLine(to: CGPoint(x: tipX, y: y))
      arrowhead.addLine(to: CGPoint(x: tipX - sign * head, y: y + head))
      arrow.addPath(arrowhead.copy(strokingWithWidth: width, lineCap: .round, lineJoin: .round, miterLimit: 10))
      directionArrow.frame = bounds
      directionArrow.path = arrow
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
