import UIKit

class ChartXAxisView: UIView {
  private static let minLabelSpacing: CGFloat = 4

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
      return (chartData.chartX(forDistance: tick.value) - CGFloat(lower)) / CGFloat(upper - lower)
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
    for (label, position) in zip(labels, positions) {
      let size = label.intrinsicContentSize
      let x: CGFloat
      // Only boundary ticks are edge-aligned; interior labels stay centered on their distance.
      if abs(position) < 1e-8 {
        x = 0
      } else if abs(position - 1) < 1e-8 {
        x = bounds.width - size.width
      } else {
        x = bounds.width * position - size.width / 2
      }
      label.frame = CGRect(origin: CGPoint(x: x, y: 0), size: size)
      label.isHidden = x < 0 || label.frame.maxX > bounds.width
    }

    // Thin the axis uniformly, including when edge-aligned labels need more room.
    let labelStride = (1 ... max(1, labels.count)).first { step in
      var previousMaxX = -CGFloat.infinity
      for index in stride(from: 0, to: labels.count, by: step) where !labels[index].isHidden {
        let frame = labels[index].frame
        guard frame.minX >= previousMaxX + Self.minLabelSpacing else { return false }
        previousMaxX = frame.maxX
      }
      return true
    }!
    for (index, label) in labels.enumerated() {
      label.isHidden = label.isHidden || index % labelStride != 0
    }
  }
}
