import UIKit

enum ChartAnimation: TimeInterval {
  case none = 0.0
  case animated = 0.3
  case interactive = 0.1
}

public class ChartView: UIView {
  private static let selectedPointCaptureRadius: CGFloat = 22

  let chartsContainerView = ExpandedTouchView()
  let yAxisView = ChartYAxisView()
  let xAxisView = ChartXAxisView()
  let chartInfoView = ChartInfoView()
  let segmentLinesView = ChartSegmentLinesView()
  var lineViews: [ChartLineView] = []

  private var tapGR: UITapGestureRecognizer!
  private var panStartPoint = 0
  private var panGR: UIPanGestureRecognizer!
  private var pinchStartLower = 0
  private var pinchStartUpper = 0
  private var pinchGR: UIPinchGestureRecognizer!

  public var isZoomEnabled = true {
    didSet {
      pinchGR.isEnabled = isZoomEnabled
      panGR.isEnabled = isZoomEnabled
    }
  }

  public var myPosition: Double = -1 {
    didSet {
      setMyPosition(myPosition)
    }
  }

  public var infoBackgroundColor: UIColor = .white {
    didSet {
      chartInfoView.infoBackgroundColor = infoBackgroundColor
      yAxisView.textBackgroundColor = infoBackgroundColor.withAlphaComponent(0.7)
    }
  }

  public var infoShadowColor: UIColor = .black {
    didSet {
      chartInfoView.infoShadowColor = infoShadowColor
    }
  }

  public var infoShadowOpacity: Float = 0.25 {
    didSet {
      chartInfoView.infoShadowOpacity = infoShadowOpacity
    }
  }

  public var font: UIFont = .systemFont(ofSize: 12, weight: .regular) {
    didSet {
      xAxisView.font = font
      yAxisView.font = font
      chartInfoView.font = font
    }
  }

  public var textColor: UIColor = .init(white: 0, alpha: 0.2) {
    didSet {
      xAxisView.textColor = textColor
      yAxisView.textColor = textColor
      chartInfoView.textColor = textColor
    }
  }

  public var gridColor: UIColor = .init(white: 0, alpha: 0.2) {
    didSet {
      yAxisView.gridColor = gridColor
      segmentLinesView.lineColor = gridColor
    }
  }

  override public var backgroundColor: UIColor? {
    didSet {
      chartInfoView.tooltipBackgroundColor = backgroundColor?.resolvedColor(with: traitCollection) ?? .white
    }
  }

  public var chartData: ChartPresentationData! {
    didSet {
      lineViews.forEach { $0.removeFromSuperview() }
      lineViews.removeAll()
      for i in (0 ..< chartData.linesCount).reversed() {
        let line = chartData.lineAt(i)
        let v = ChartLineView()
        v.clipsToBounds = true
        v.chartLine = line
        v.lineWidth = 3
        v.frame = chartsContainerView.bounds
        v.autoresizingMask = [.flexibleWidth, .flexibleHeight]
        chartsContainerView.insertSubview(v, belowSubview: segmentLinesView)
        lineViews.insert(v, at: 0)
      }

      segmentLinesView.setSegmentDistances(chartData.segmentDistances, chartData: chartData)

      yAxisView.frame = chartsContainerView.bounds
      yAxisView.autoresizingMask = [.flexibleWidth, .flexibleHeight]
      yAxisView.transform = CGAffineTransform.identity.scaledBy(x: 1, y: -1)
      chartsContainerView.addSubview(yAxisView)

      chartInfoView.frame = chartsContainerView.bounds
      chartInfoView.autoresizingMask = [.flexibleWidth, .flexibleHeight]
      chartInfoView.delegate = self
      chartInfoView.textColor = textColor
      chartsContainerView.addSubview(chartInfoView)

      xAxisView.chartData = chartData
      xAxisView.setBounds(lower: 0, upper: chartData.pointsCount - 1)
      updateCharts()
    }
  }

  public var isChartViewInfoHidden: Bool = false {
    didSet {
      chartInfoView.isHidden = isChartViewInfoHidden
      chartInfoView.isUserInteractionEnabled = !isChartViewInfoHidden
    }
  }

  public typealias OnSelectedPointChangedClosure = (_ distance: Double) -> Void
  public var onSelectedPointChanged: OnSelectedPointChangedClosure?
  public var onSelectionFinished: OnSelectedPointChangedClosure?
  public var onSelectionCancelled: (() -> Void)?

  override init(frame: CGRect) {
    super.init(frame: frame)
    setup()
  }

  required init?(coder aDecoder: NSCoder) {
    super.init(coder: aDecoder)
    setup()
  }

  private func setup() {
    isUserInteractionEnabled = false

    xAxisView.font = font
    xAxisView.textColor = textColor
    yAxisView.font = font
    yAxisView.textColor = textColor
    yAxisView.gridColor = textColor
    chartInfoView.font = font
    chartInfoView.tooltipBackgroundColor = backgroundColor ?? .white
    yAxisView.textBackgroundColor = infoBackgroundColor.withAlphaComponent(0.7)
    segmentLinesView.lineColor = gridColor

    tapGR = UITapGestureRecognizer(target: self, action: #selector(onTap(_:)))
    tapGR.require(toFail: chartInfoView.selectionGestureRecognizer)
    chartsContainerView.addGestureRecognizer(tapGR)
    panGR = UIPanGestureRecognizer(target: self, action: #selector(onPan(_:)))
    panGR.delegate = self
    chartsContainerView.addGestureRecognizer(panGR)
    pinchGR = UIPinchGestureRecognizer(target: self, action: #selector(onPinch(_:)))
    chartsContainerView.addGestureRecognizer(pinchGR)
    addSubview(chartsContainerView)

    segmentLinesView.frame = chartsContainerView.bounds
    segmentLinesView.autoresizingMask = [.flexibleWidth, .flexibleHeight]
    chartsContainerView.addSubview(segmentLinesView)

    addSubview(xAxisView)
  }

  public func setSelectedPoint(_ x: Double) {
    let routeLength = chartData.distance(forChartX: CGFloat(chartData.pointsCount - 1))
    var upper = chartData.xAxisValueAt(CGFloat(xAxisView.upperBound))
    var lower = chartData.xAxisValueAt(CGFloat(xAxisView.lowerBound))
    let rangeLength = upper - lower
    if x < lower || x > upper, routeLength > 0 {
      let current = Double(chartInfoView.infoX) * rangeLength + lower
      let currentChartX = chartData.chartX(forDistance: current)
      let targetChartX = chartData.chartX(forDistance: x)
      let dIdx = Int(round(targetChartX - currentChartX))
      var lowerIdx = xAxisView.lowerBound + dIdx
      var upperIdx = xAxisView.upperBound + dIdx
      if lowerIdx < 0 {
        upperIdx -= lowerIdx
        lowerIdx = 0
      } else if upperIdx >= chartData.pointsCount {
        lowerIdx -= upperIdx - chartData.pointsCount + 1
        upperIdx = chartData.pointsCount - 1
      }
      setViewport(lower: lowerIdx, upper: upperIdx, notifySelection: false)
      lower = chartData.xAxisValueAt(CGFloat(xAxisView.lowerBound))
      upper = chartData.xAxisValueAt(CGFloat(xAxisView.upperBound))
    }
    let currentRangeLength = upper - lower
    guard currentRangeLength > 0 else { return }
    chartInfoView.infoX = max(0, min(1, CGFloat((x - lower) / currentRangeLength)))
  }

  private func setMyPosition(_ x: Double) {
    let upper = chartData.xAxisValueAt(CGFloat(xAxisView.upperBound))
    let lower = chartData.xAxisValueAt(CGFloat(xAxisView.lowerBound))
    let rangeLength = upper - lower
    guard rangeLength > 0 else { return }
    chartInfoView.myPositionX = CGFloat((x - lower) / rangeLength)
  }

  func setViewport(lower: Int, upper: Int, notifySelection: Bool = true) {
    guard lower < upper else { return }
    xAxisView.setBounds(lower: lower, upper: upper)
    updateCharts()
    chartInfoView.update()
    setMyPosition(myPosition)
    if notifySelection {
      onSelectedPointChanged?(selectedDistance)
    }
  }

  override public func layoutSubviews() {
    super.layoutSubviews()

    let (axisFrame, chartFrame) = bounds.divided(atDistance: 26, from: .maxYEdge)
    xAxisView.frame = axisFrame
    chartsContainerView.frame = chartFrame
  }

  override public func point(inside point: CGPoint, with _: UIEvent?) -> Bool {
    let rect = bounds.insetBy(dx: -30, dy: 0)
    return rect.contains(point)
  }

  @objc func onTap(_ sender: UITapGestureRecognizer) {
    guard sender.state == .ended else {
      return
    }
    let point = sender.location(in: chartInfoView)
    updateCharts(animationStyle: .none)
    chartInfoView.update(point.x)
    chartInfoView(chartInfoView, didMoveToPoint: point.x)
    notifySelectionFinished()
  }

  @objc func onPinch(_ sender: UIPinchGestureRecognizer) {
    guard isZoomEnabled, chartData != nil else { return }
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
    let upper = min(pinchStartUpper - dx, chartData.pointsCount - 1)

    guard upper - lower > max(1, chartData.pointsCount / 10) else {
      return
    }

    setViewport(lower: lower, upper: upper)
  }

  @objc func onPan(_ sender: UIPanGestureRecognizer) {
    guard chartData != nil else { return }
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
    if lower < 0 || upper > chartData.pointsCount - 1 {
      return
    }

    setViewport(lower: lower, upper: upper)
  }

  func updateCharts(animationStyle: ChartAnimation = .none) {
    var lower = CGFloat(Int.max)
    var upper = CGFloat(Int.min)
    let minX = CGFloat(xAxisView.lowerBound)
    let maxX = CGFloat(xAxisView.upperBound)

    for i in 0 ..< chartData.linesCount {
      // Compare in the X coordinates of the drawn path: converting the viewport to distances can round past
      // a vertex on the viewport edge, e.g. both vertices of a segment break.
      let line = chartData.lineAt(i)
      let values = line.values.map { ChartValue(xValues: $0.x * line.xScale, y: $0.y) }
      // Include the interpolated edges when the viewport cuts through a segment.
      let lowerAltitude = values.interpolatedAltitude(at: minX)
      let upperAltitude = values.interpolatedAltitude(at: maxX)
      lower = min(lower, lowerAltitude, upperAltitude)
      upper = max(upper, lowerAltitude, upperAltitude)
      let start = values.firstIndexAtOrAfter(minX, by: \.x) ?? values.endIndex
      for item in values[start...].prefix(while: { $0.x <= maxX }) {
        upper = max(item.y, upper)
        lower = min(item.y, lower)
      }
    }

    // Keep flat profiles drawable even when both heights fall on an axis tick.
    let padding: CGFloat = upper == lower ? 1 : round((upper - lower) / 10)
    let paddedLower = lower - padding
    let adjustedLower = lower >= 0 ? max(0, paddedLower) : paddedLower
    lower = chartData.formatter.yAxisLowerBound(from: adjustedLower)
    upper = chartData.formatter.yAxisUpperBound(from: upper + padding)
    let steps = chartData.formatter.yAxisSteps(lowerBound: lower, upperBound: upper)

    if yAxisView.upperBound != upper || yAxisView.lowerBound != lower {
      yAxisView.setBounds(lower: lower,
                          upper: upper,
                          lowerLabel: chartData.formatter.yAxisString(from: Double(lower)),
                          upperLabel: chartData.formatter.yAxisString(from: Double(upper)),
                          steps: steps,
                          animationStyle: animationStyle)
    }

    for lineView in lineViews {
      lineView.setViewport(minX: xAxisView.lowerBound,
                           maxX: xAxisView.upperBound,
                           minY: lower,
                           maxY: upper,
                           animationStyle: animationStyle)
    }
    segmentLinesView.setViewport(minX: xAxisView.lowerBound, maxX: xAxisView.upperBound)
  }

  private var isChartZoomed: Bool {
    guard let chartData else { return false }
    return xAxisView.lowerBound > 0 || xAxisView.upperBound < chartData.pointsCount - 1
  }

  private func shouldStartSelecting(at pointX: CGFloat) -> Bool {
    guard chartData != nil, chartInfoView.bounds.width > 0 else { return false }
    if !isChartZoomed {
      return true
    }

    let clampedPointX = max(chartInfoView.bounds.minX, min(chartInfoView.bounds.maxX, pointX))
    let selectedPointX = chartInfoView.infoX * chartInfoView.bounds.width
    return abs(clampedPointX - selectedPointX) <= Self.selectedPointCaptureRadius
  }

  private func chartX(atFraction fraction: CGFloat) -> CGFloat {
    fraction * CGFloat(xAxisView.upperBound - xAxisView.lowerBound) + CGFloat(xAxisView.lowerBound)
  }

  private var selectedDistance: Double {
    chartData.distance(forChartX: chartX(atFraction: chartInfoView.infoX))
  }

  private func notifySelectionFinished() {
    guard chartData != nil else { return }
    onSelectionFinished?(selectedDistance)
  }
}

extension ChartView: ChartInfoViewDelegate {
  func chartInfoViewDidFinishSelection(_: ChartInfoView) {
    notifySelectionFinished()
  }

  func chartInfoViewDidCancelSelection(_: ChartInfoView) {
    onSelectionCancelled?()
  }

  func chartInfoView(_ view: ChartInfoView, didMoveToPoint pointX: CGFloat) {
    guard view.bounds.width > 0 else { return }
    onSelectedPointChanged?(chartData.distance(forChartX: chartX(atFraction: pointX / view.bounds.width)))
  }

  func chartInfoView(_: ChartInfoView, shouldStartSelectingAtPoint pointX: CGFloat) -> Bool {
    shouldStartSelecting(at: pointX)
  }

  func chartInfoView(_ view: ChartInfoView, infoAtPointX pointX: CGFloat) -> (String, [ChartLineInfo])? {
    guard let chartData, view.bounds.width > 0 else { return nil }
    let chartX = chartX(atFraction: pointX / view.bounds.width)
    guard chartX >= 0,
          chartX <= CGFloat(chartData.pointsCount - 1) else { return nil }
    let distance = CGFloat(chartData.distance(forChartX: chartX))
    let label = chartData.labelAt(chartX)

    var result: [ChartLineInfo] = []
    for i in 0 ..< chartData.linesCount {
      let line = chartData.lineAt(i)
      guard line.type != .lineArea else { continue }
      let y: CGFloat
      if let section = line.highlighted, abs(section.entry.x - chartX) <= 4 * max(chartX.ulp, section.entry.x.ulp) {
        // Two gap endpoints can share an X coordinate but have different heights.
        y = section.entry.y + line.minY
      } else {
        y = line.values.interpolatedAltitude(at: distance)
      }
      let py = round(chartsContainerView.bounds.height * CGFloat(y - yAxisView.lowerBound) /
        CGFloat(yAxisView.upperBound - yAxisView.lowerBound))
      let markerY = chartsContainerView.convert(CGPoint(x: 0, y: py), to: view).y
      result.append(ChartLineInfo(color: line.color,
                                  point: CGPoint(x: pointX, y: markerY),
                                  formattedValue: chartData.formatter.yAxisString(from: Double(y))))
    }

    return (label, result)
  }
}

extension ChartView: UIGestureRecognizerDelegate {
  override public func gestureRecognizerShouldBegin(_ gestureRecognizer: UIGestureRecognizer) -> Bool {
    guard gestureRecognizer === panGR else { return true }
    guard isChartZoomed else { return false }
    let pointX = gestureRecognizer.location(in: chartInfoView).x
    return !shouldStartSelecting(at: pointX)
  }
}
