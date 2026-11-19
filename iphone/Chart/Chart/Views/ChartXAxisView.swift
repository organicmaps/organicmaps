import UIKit

class ChartXAxisView: UIView {
  private static let minLabelSpacing: CGFloat = 4
  private static let tickHeight: CGFloat = 3
  private static let labelTopInset: CGFloat = 5

  override class var layerClass: AnyClass { CAShapeLayer.self }

  var lowerBound = 0
  var upperBound = 0
  var chartData: ChartPresentationData!

  var font: UIFont = .systemFont(ofSize: 12, weight: .regular) {
    didSet {
      labels.forEach { $0.font = font }
      setNeedsLayout()
    }
  }

  var textColor: UIColor = .init(white: 0, alpha: 0.3) {
    didSet {
      labels.forEach { $0.textColor = textColor }
      setNeedsLayout()
    }
  }

  private var labels: [UILabel] = []
  private var positions: [CGFloat] = []

  func setBounds(lower: Int, upper: Int) {
    lowerBound = lower
    upperBound = upper
    let lowerDistance = chartData.distance(forChartX: CGFloat(lower))
    let upperDistance = chartData.distance(forChartX: CGFloat(upper))
    let ticks = chartData.formatter.xAxisLabels(lowerBound: lowerDistance, upperBound: upperDistance)
    positions = ticks.map { tick in
      guard upperDistance > lowerDistance else { return 0 }
      return CGFloat((tick.value - lowerDistance) / (upperDistance - lowerDistance))
    }
    labels.forEach { $0.removeFromSuperview() }
    labels = ticks.map { tick in
      let label = UILabel()
      label.font = font
      label.textColor = textColor
      label.text = tick.text
      addSubview(label)
      return label
    }
    setNeedsLayout()
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    // Tick generation tolerates conversion roundoff at the viewport boundaries.
    let tickPositions = positions.map { bounds.width * min(max($0, 0), 1) }
    for (label, tickX) in zip(labels, tickPositions) {
      let size = label.intrinsicContentSize
      // Keep the text inside the chart; the separate tick marks its actual distance.
      let x = min(max(tickX - size.width / 2, 0), max(0, bounds.width - size.width))
      label.frame = CGRect(origin: CGPoint(x: x, y: Self.labelTopInset), size: size)
      label.isHidden = size.width > bounds.width
    }

    // Thin uniformly from the last tick so the end label survives collisions.
    var labelStride = 1
    while !labelsFit(stride: labelStride) {
      labelStride += 1
    }
    for (index, label) in labels.enumerated() {
      label.isHidden = label.isHidden || (labels.count - 1 - index) % labelStride != 0
    }

    let path = UIBezierPath()
    for (label, tickX) in zip(labels, tickPositions) where !label.isHidden {
      path.move(to: CGPoint(x: tickX, y: 0))
      path.addLine(to: CGPoint(x: tickX, y: Self.tickHeight))
    }
    let ticksLayer = layer as! CAShapeLayer
    ticksLayer.path = path.cgPath
    ticksLayer.fillColor = nil
    ticksLayer.strokeColor = textColor.resolvedColor(with: traitCollection).cgColor
  }

  private func labelsFit(stride step: Int) -> Bool {
    var nextMinX = CGFloat.infinity
    for index in stride(from: labels.count - 1, through: 0, by: -step) where !labels[index].isHidden {
      let frame = labels[index].frame
      guard frame.maxX + Self.minLabelSpacing <= nextMinX else { return false }
      nextMinX = frame.minX
    }
    return true
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    if traitCollection.hasDifferentColorAppearance(comparedTo: previousTraitCollection) {
      setNeedsLayout()
    }
  }
}
