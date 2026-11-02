import UIKit

private class ChartXAxisInnerView: UIView {
  private static let minLabelSpacing: CGFloat = 4

  var labels: [UILabel] = []

  var font: UIFont = .systemFont(ofSize: 12, weight: .regular) {
    didSet {
      labels.forEach { $0.font = font }
    }
  }

  var textColor: UIColor = .init(white: 0, alpha: 0.3) {
    didSet {
      labels.forEach { $0.textColor = textColor }
    }
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    updateLabels()
  }

  private func makeLabel(text: String) -> UILabel {
    let label = UILabel()
    label.font = font
    label.textColor = textColor
    label.text = text
    label.frame = CGRect(x: 0, y: 0, width: 60, height: 15)
    return label
  }

  func setLabels(_ steps: [String]) {
    labels.forEach { $0.removeFromSuperview() }
    labels.removeAll()

    for i in 0 ..< steps.count {
      let step = steps[i]
      let label = makeLabel(text: step)
      if i == 0 {
        label.textAlignment = .left
      } else if i == steps.count - 1 {
        label.textAlignment = .right
      } else {
        label.textAlignment = .center
      }
      labels.append(label)
      addSubview(label)
    }

    setNeedsLayout()
  }

  private func updateLabels() {
    guard bounds.width > 0 else { return }
    for i in 0 ..< labels.count {
      let fraction = labels.count > 1 ? CGFloat(i) / CGFloat(labels.count - 1) : 0
      let x = bounds.width * fraction
      let l = labels[i]
      var f = l.frame
      switch l.textAlignment {
      case .left: f.origin.x = x
      case .right: f.origin.x = x - f.width
      default: f.origin.x = x - f.width / 2
      }
      f.origin.y = 0
      l.frame = f
    }

    // Hide interior labels that would overlap an edge label.
    guard labels.count > 2, let first = labels.first, let last = labels.last else { return }
    let minX = first.frame.minX + first.intrinsicContentSize.width + Self.minLabelSpacing
    let maxX = last.frame.maxX - last.intrinsicContentSize.width - Self.minLabelSpacing
    for l in labels.dropFirst().dropLast() {
      let halfWidth = l.intrinsicContentSize.width / 2
      l.isHidden = l.frame.midX - halfWidth < minX || l.frame.midX + halfWidth > maxX
    }
  }
}

class ChartXAxisView: UIView {
  var lowerBound = 0
  var upperBound = 0
  var chartData: ChartPresentationData!

  var font: UIFont = .systemFont(ofSize: 12, weight: .regular) {
    didSet {
      labelsView?.font = font
    }
  }

  var textColor: UIColor = .init(white: 0, alpha: 0.3) {
    didSet {
      labelsView?.textColor = textColor
    }
  }

  private var labelsView: ChartXAxisInnerView?

  func setBounds(lower: Int, upper: Int) {
    lowerBound = lower
    upperBound = upper

    // Keep a single label if even the two edge labels repeat (e.g. a zero-length profile).
    var labels = [chartData.labelAt(CGFloat(lower))]
    // Reduce tick density when formatting would repeat the same distance.
    for intervalCount in (1 ... 5).reversed() {
      let steps = (0 ... intervalCount).map { i in
        let chartX = CGFloat(lower) + CGFloat(upper - lower) * CGFloat(i) / CGFloat(intervalCount)
        return chartData.labelAt(chartX)
      }
      if Set(steps).count == steps.count {
        labels = steps
        break
      }
    }

    let lv = ChartXAxisInnerView()
    lv.frame = bounds
    lv.font = font
    lv.textColor = textColor
    lv.autoresizingMask = [.flexibleWidth, .flexibleHeight]
    addSubview(lv)

    if let labelsView = labelsView {
      labelsView.removeFromSuperview()
    }

    lv.setLabels(labels)
    labelsView = lv
  }
}
