import UIKit

final class ChartSegmentLinesView: UIView {
  override class var layerClass: AnyClass { CAShapeLayer.self }

  private var minX = 0
  private var maxX = 0
  private var segmentChartX: [CGFloat] = []

  var lineColor: UIColor = .gray {
    didSet {
      shapeLayer.strokeColor = lineColor.cgColor
    }
  }

  var lineWidth: CGFloat = 2.0 {
    didSet {
      shapeLayer.lineWidth = lineWidth
    }
  }

  var dashPattern: [NSNumber] = [8, 4] {
    didSet {
      shapeLayer.lineDashPattern = dashPattern
    }
  }

  override init(frame: CGRect) {
    super.init(frame: frame)
    isUserInteractionEnabled = false
    backgroundColor = .clear
    shapeLayer.fillColor = UIColor.clear.cgColor
    shapeLayer.strokeColor = lineColor.cgColor
    shapeLayer.lineWidth = lineWidth
    shapeLayer.lineDashPattern = dashPattern
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError()
  }

  private var shapeLayer: CAShapeLayer {
    layer as! CAShapeLayer
  }

  func setSegmentDistances(_ distances: [Double], chartData: ChartPresentationData) {
    segmentChartX = distances.map { chartData.chartX(forDistance: $0) }
    updateGraph()
  }

  func setViewport(minX: Int, maxX: Int) {
    guard minX < maxX else { return }
    self.minX = minX
    self.maxX = maxX
    updateGraph()
  }

  private func updateGraph() {
    guard bounds.width > 0, bounds.height > 0, minX < maxX else {
      shapeLayer.path = nil
      return
    }

    let path = UIBezierPath()
    let xScale = bounds.width / CGFloat(maxX - minX)
    let xTranslate = -bounds.width * CGFloat(minX) / CGFloat(maxX - minX)

    for chartX in segmentChartX {
      let x = chartX * xScale + xTranslate
      guard x >= 0, x <= bounds.width else { continue }
      path.move(to: CGPoint(x: x, y: 0))
      path.addLine(to: CGPoint(x: x, y: bounds.height))
    }

    shapeLayer.path = path.cgPath
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    updateGraph()
  }
}
