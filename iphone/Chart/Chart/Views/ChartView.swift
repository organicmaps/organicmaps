import UIKit

enum ChartAnimation: TimeInterval {
  case none = 0.0
  case animated = 0.3
  case interactive = 0.1
}

public class ChartView: UIView {
  let chartsContainerView = UIView()
  let chartPreviewView = ChartPreviewView()
  let yAxisView = ChartYAxisView()
  let xAxisView = ChartXAxisView()
  let chartInfoView = ChartInfoView()
  var lineViews: [ChartLineView] = []

  private var panStartPoint = 0
  private var panGR: UIPanGestureRecognizer!
  private var pinchStartLower = 0
  private var pinchStartUpper = 0
  private var pinchGR: UIPinchGestureRecognizer!

  public var previewSelectorColor: UIColor = UIColor.lightGray.withAlphaComponent(0.9) {
    didSet {
      chartPreviewView.selectorColor = previewSelectorColor
    }
  }

  public var previewTintColor: UIColor = UIColor.lightGray.withAlphaComponent(0.5) {
    didSet {
      chartPreviewView.selectorTintColor = previewTintColor
    }
  }

  public var headerTextColor: UIColor = UIColor.lightGray {
    didSet {
      chartInfoView.textColor = headerTextColor
    }
  }

  public var gridTextColor: UIColor = UIColor(white: 0, alpha: 0.2) {
    didSet {
      xAxisView.gridColor = gridTextColor
      yAxisView.gridColor = gridTextColor
    }
  }

  public var gridLineColor: UIColor = UIColor(white: 0, alpha: 0.2) {
    didSet {
      yAxisView.gridLineColor = gridLineColor
    }
  }

  public var bgColor: UIColor = UIColor.white {
    didSet {
      chartInfoView.bgColor = bgColor
    }
  }

  weak var headerUpdateTimer: Timer?

  public var rasterize = false {
    didSet {
      lineViews.forEach {
        $0.layer.shouldRasterize = rasterize
        $0.layer.rasterizationScale = UIScreen.main.scale
      }
    }
  }

  public var chartData: ChartPresentationData! {
    didSet {
      lineViews.forEach { $0.removeFromSuperview() }
      lineViews.removeAll()
      for i in (0..<chartData.linesCount).reversed() {
        let line = chartData.lineAt(i)
        let v = ChartLineView()
        v.clipsToBounds = true
        v.chartLine = line
        v.lineWidth = 3
        v.frame = chartsContainerView.bounds
        v.autoresizingMask = [.flexibleWidth, .flexibleHeight]
        chartsContainerView.addSubview(v)
        lineViews.insert(v, at: 0)
      }

      yAxisView.frame = chartsContainerView.bounds
      yAxisView.autoresizingMask = [.flexibleWidth, .flexibleHeight]
      yAxisView.transform = CGAffineTransform.identity.scaledBy(x: 1, y: -1)
      chartsContainerView.addSubview(yAxisView)

      chartInfoView.frame = chartsContainerView.bounds
      chartInfoView.autoresizingMask = [.flexibleWidth, .flexibleHeight]
      chartInfoView.delegate = self
      chartInfoView.bgColor = bgColor
      chartInfoView.textColor = headerTextColor
      chartsContainerView.addSubview(chartInfoView)

      xAxisView.values = chartData.labels
      chartPreviewView.chartData = chartData
      xAxisView.setBounds(lower: chartPreviewView.minX, upper: chartPreviewView.maxX)
      updateCharts()
    }
  }

  public typealias OnSelectePointChangedClosure = (_ px: CGFloat) -> Void
  public var onSelectedPointChanged: OnSelectePointChangedClosure?

  override init(frame: CGRect) {
    super.init(frame: frame)
    setup()
  }

  required init?(coder aDecoder: NSCoder) {
    super.init(coder: aDecoder)
    setup()
  }

  private func setup() {
    xAxisView.gridColor = gridTextColor
    yAxisView.gridColor = gridTextColor
    yAxisView.gridLineColor = gridTextColor
    chartInfoView.bgColor = bgColor
    chartPreviewView.selectorTintColor = previewTintColor
    chartPreviewView.selectorColor = previewSelectorColor

    panGR = UIPanGestureRecognizer(target: self, action: #selector(onPan(_:)))
    chartsContainerView.addGestureRecognizer(panGR)
    pinchGR = UIPinchGestureRecognizer(target: self, action: #selector(onPinch(_:)))
    chartsContainerView.addGestureRecognizer(pinchGR)
    addSubview(chartsContainerView)
    addSubview(chartPreviewView)
    chartPreviewView.delegate = self
    addSubview(xAxisView)
  }

  public func setSelectedPoint(_ x: Double) {
    let routeLength = chartData.xAxisValues.last!
    chartInfoView.infoX = CGFloat(x / routeLength)
  }

  override public func layoutSubviews() {
    super.layoutSubviews()

    let previewFrame = CGRect(x: bounds.minX, y: bounds.maxY - 30, width: bounds.width, height: 30)
    chartPreviewView.frame = previewFrame

    let xAxisFrame = CGRect(x: bounds.minX, y: bounds.maxY - previewFrame.height - 26, width: bounds.width, height: 26)
    xAxisView.frame = xAxisFrame

    let chartsFrame = CGRect(x: bounds.minX,
                             y: bounds.minY,
                             width: bounds.width,
                             height: bounds.maxY - previewFrame.height - xAxisFrame.height)
    chartsContainerView.frame = chartsFrame
  }

  @objc func onPinch(_ sender: UIPinchGestureRecognizer) {
    if sender.state == .began {
      pinchStartLower = xAxisView.lowerBound
      pinchStartUpper = xAxisView.upperBound
    }

    if sender.state != .changed {
      return
    }

    let rangeLength = CGFloat(pinchStartUpper - pinchStartLower)
    let dx = Int(round((rangeLength * sender.scale - rangeLength) / 2))
    let lower = max(pinchStartLower + dx, 0)
    let upper = min(pinchStartUpper - dx, chartData.labels.count - 1)

    if upper - lower < chartData.labels.count / 10 {
      return
    }
    
    chartPreviewView.setX(min: lower, max: upper)
    xAxisView.setBounds(lower: lower, upper: upper)
    updateCharts(animationStyle: .none)
    chartInfoView.update()
  }

  @objc func onPan(_ sender: UIPanGestureRecognizer) {
    let t = sender.translation(in: chartsContainerView)
    if sender.state == .began {
      panStartPoint = xAxisView.lowerBound
    }

    if sender.state != .changed {
      return
    }

    let dx = Int(round(t.x / chartsContainerView.bounds.width * CGFloat(xAxisView.upperBound - xAxisView.lowerBound)))
    let lower = panStartPoint - dx
    let upper = lower + xAxisView.upperBound - xAxisView.lowerBound
    if lower < 0 || upper > chartData.labels.count - 1 {
      return
    }
    
    chartPreviewView.setX(min: lower, max: upper)
    xAxisView.setBounds(lower: lower, upper: upper)
    updateCharts(animationStyle: .none)
    chartInfoView.update()
  }

  func updateCharts(animationStyle: ChartAnimation = .none) {
    var lower = CGFloat(Int.max)
    var upper = CGFloat(Int.min)

    for i in 0..<chartData.linesCount {
      let line = chartData.lineAt(i)
      let subrange = line.aggregatedValues[xAxisView.lowerBound...xAxisView.upperBound]
      subrange.forEach {
        upper = max($0, upper)
        if line.type == .line || line.type == .lineArea {
          lower = min($0, lower)
        }
      }
    }

    let padding = round((upper - lower) / 10)
    lower = max(0, lower - padding)
    upper = upper + padding

    let stepsCount = 3
    let step = ceil((upper - lower) / CGFloat(stepsCount))
    upper = lower + step * CGFloat(stepsCount)
    var steps: [CGFloat] = []
    for i in 0..<stepsCount {
      steps.append(lower + step * CGFloat(i))
    }

    if yAxisView.upperBound != upper || yAxisView.lowerBound != lower {
      yAxisView.setBounds(lower: lower,
                          upper: upper,
                          lowerLabel: chartData.formatter.altitudeString(from: Double(lower)),
                          upperLabel: chartData.formatter.altitudeString(from: Double(upper)),
                          steps: steps,
                          animationStyle: animationStyle)
    }

    lineViews.forEach {
      $0.setViewport(minX: xAxisView.lowerBound,
                     maxX: xAxisView.upperBound,
                     minY: lower,
                     maxY: upper,
                     animationStyle: animationStyle)
    }
  }
}

extension ChartView: ChartPreviewViewDelegate {
  func chartPreviewView(_ view: ChartPreviewView, didChangeMinX minX: Int, maxX: Int) {
    xAxisView.setBounds(lower: minX, upper: maxX)
    updateCharts(animationStyle: .none)
    chartInfoView.update()
  }
}

extension ChartView: ChartInfoViewDelegate {
  func chartInfoView(_ view: ChartInfoView, didCaptureInfoView captured: Bool) {
    panGR.isEnabled = !captured
  }

  func chartInfoView(_ view: ChartInfoView, infoAtPointX pointX: CGFloat) -> (String, [ChartLineInfo])? {
    let p = convert(CGPoint(x: pointX, y: 0), from: view)
    let x = (p.x / bounds.width) * CGFloat(xAxisView.upperBound - xAxisView.lowerBound) + CGFloat(xAxisView.lowerBound)
    let x1 = Int(floor(x))
    let x2 = Int(ceil(x))
    guard x1 < chartData.labels.count && x >= 0 else { return nil }
    let label = chartData.labelAt(x)

    var result: [ChartLineInfo] = []
    for i in 0..<chartData.linesCount {
      let line = chartData.lineAt(i)
      guard line.type != .lineArea else { continue }
      let y1 = line.values[x1]
      let y2 = line.values[x2]

      let dx = x - CGFloat(x1)
      let y = dx * (y2 - y1) + y1
      let py = round(chartsContainerView.bounds.height * CGFloat(y - yAxisView.lowerBound) /
        CGFloat(yAxisView.upperBound - yAxisView.lowerBound))

      let v1 = line.originalValues[x1]
      let v2 = line.originalValues[x2]
      let v = round(dx * CGFloat(v2 - v1)) + CGFloat(v1)
      onSelectedPointChanged?(x)
      result.append(ChartLineInfo(name: line.name,
                                  color: line.color,
                                  point: chartsContainerView.convert(CGPoint(x: p.x, y: py), to: view),
                                  formattedValue: chartData.formatter.altitudeString(from: Double(v))))
    }

    return (label, result)
  }
}
