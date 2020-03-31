import UIKit

fileprivate class ChartXAxisInnerView: UIView {
  var lowerBound = 0
  var upperBound = 0
  var steps: [String] = []
  var labels: [UILabel] = []

  var font: UIFont = UIFont.systemFont(ofSize: 12, weight: .regular) {
    didSet {
      labels.forEach { $0.font = font }
    }
  }

  var textColor: UIColor = UIColor(white: 0, alpha: 0.3) {
    didSet {
      labels.forEach { $0.textColor = textColor }
    }
  }

  override var frame: CGRect {
    didSet {
      if upperBound > 0 {
        updateLabels()
      }
    }
  }

  func makeLabel(text: String) -> UILabel {
    let label = UILabel()
    label.font = font
    label.textColor = textColor
    label.text = text
    label.frame = CGRect(x: 0, y: 0, width: 50, height: 15)
    return label
  }

  func setBounds(lower: Int, upper: Int, steps: [String]) {
    lowerBound = lower
    upperBound = upper
    self.steps = steps

    labels.forEach { $0.removeFromSuperview() }
    labels.removeAll()

    for i in 0..<steps.count {
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

    updateLabels()
  }

  func updateLabels() {
    let step = CGFloat(upperBound - lowerBound) / CGFloat(labels.count - 1)
    for i in 0..<labels.count {
      let x = bounds.width * step * CGFloat(i) / CGFloat(upperBound - lowerBound)
      let l = labels[i]
      var f = l.frame
      let adjust = bounds.width > 0 ? x / bounds.width : 0
      f.origin = CGPoint(x: x - f.width * adjust, y: 0)
      l.frame = f.integral
    }
  }
}

class ChartXAxisView: UIView {
  var lowerBound = 0
  var upperBound = 0

  var values: [String] = []

  var font: UIFont = UIFont.systemFont(ofSize: 12, weight: .regular) {
    didSet {
      labelsView?.font = font
    }
  }

  var textColor: UIColor = UIColor(white: 0, alpha: 0.3) {
    didSet {
      labelsView?.textColor = textColor
    }
  }

  private var labelsView: ChartXAxisInnerView?

  func setBounds(lower: Int, upper: Int) {
    lowerBound = lower
    upperBound = upper
    let step = CGFloat(upper - lower) / 5

    var steps: [String] = []
    for i in 0..<5 {
      let x = lower + Int(round(step * CGFloat(i)))
      steps.append(values[x])
    }
    steps.append(values[upper])

    let lv = ChartXAxisInnerView()
    lv.frame = bounds
    lv.textColor = textColor
    lv.autoresizingMask = [.flexibleWidth, .flexibleHeight]
    addSubview(lv)

    if let labelsView = labelsView {
      labelsView.removeFromSuperview()
    }

    lv.setBounds(lower: lower, upper: upper, steps: steps)
    labelsView = lv
  }
}
