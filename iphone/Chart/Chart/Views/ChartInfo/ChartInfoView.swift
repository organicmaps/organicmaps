import UIKit

struct ChartLineInfo {
  let name: String
  let color: UIColor
  let point: CGPoint
  let formattedValue: String
}

protocol ChartInfoViewDelegate: AnyObject {
  func chartInfoView(_ view: ChartInfoView, infoAtPointX pointX: CGFloat) -> (String, [ChartLineInfo])?
  func chartInfoView(_ view: ChartInfoView, didCaptureInfoView captured: Bool)
}

class ChartInfoView: UIView {
  weak var delegate: ChartInfoViewDelegate?

  private let pointInfoView = ChartPointInfoView()
  private let pointsView = ChartPointIntersectionsView(frame: CGRect(x: 0, y: 0, width: 2, height: 0))
  private let myPositionView = ChartMyPositionView(frame: CGRect(x: 50, y: 0, width: 2, height: 0))
  private var lineInfo: ChartLineInfo?

  fileprivate var captured = false {
    didSet {
      delegate?.chartInfoView(self, didCaptureInfoView: captured)
    }
  }

  var infoX: CGFloat = 0 {
    didSet {
      if bounds.width > 0 {
        let x = bounds.width * infoX
        update(x)
        updateViews(point: lineInfo!.point)
      }
    }
  }

  var bgColor: UIColor = UIColor.white

  var textColor: UIColor = UIColor.black {
    didSet {
      pointInfoView.textColor = textColor
    }
  }

  var panGR: UIPanGestureRecognizer!

  override init(frame: CGRect) {
    super.init(frame: frame)

    addSubview(myPositionView)
    myPositionView.isHidden = true
    addSubview(pointsView)
    addSubview(pointInfoView)
    isExclusiveTouch = true
    panGR = UIPanGestureRecognizer(target: self, action: #selector(onPan(_:)))
    panGR.delegate = self
    addGestureRecognizer(panGR)
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError()
  }

  override func point(inside point: CGPoint, with event: UIEvent?) -> Bool {
    bounds.insetBy(dx: -22, dy: 0).contains(point)
  }

  func update(_ x: CGFloat? = nil) {
    let x = x ?? pointsView.center.x
    guard let delegate = delegate,
      let (label, intersectionPoints) = delegate.chartInfoView(self, infoAtPointX: x) else { return }
    lineInfo = intersectionPoints[0]
    self.pointsView.updatePoints(intersectionPoints)
    pointInfoView.update(x: x, label: label, points: intersectionPoints)
    updateViews(point: lineInfo!.point)
    let y = max(pointInfoView.frame.height / 2 + 5,
                min(bounds.height - pointInfoView.frame.height / 2 - 5, bounds.height - lineInfo!.point.y));
    pointInfoView.center = CGPoint(x: pointInfoView.center.x, y: y)
    let arrowPoint = convert(CGPoint(x: 0, y: bounds.height - lineInfo!.point.y), to: pointInfoView)
    pointInfoView.arrowY = arrowPoint.y
  }

  @objc func onPan(_ sender: UIPanGestureRecognizer) {
    let x = sender.location(in: self).x
    switch sender.state {
    case .possible:
      break
    case .began:
      guard let lineInfo = lineInfo else { return }
      captured = abs(x - lineInfo.point.x) < 22
    case .changed:
      if captured {
        if x < bounds.minX || x > bounds.maxX {
          return
        }
        update(x)
        updateViews(point: lineInfo!.point)
      }
    case .ended, .cancelled, .failed:
      captured = false
    @unknown default:
      fatalError()
    }
  }

  func updateViews(point: CGPoint) {
    pointsView.alpha = 1
    pointsView.center = CGPoint(x: point.x, y: bounds.midY)
    
    let s = pointInfoView.systemLayoutSizeFitting(UIView.layoutFittingCompressedSize)
    pointInfoView.frame.size = s
    let orientationChangeX = pointInfoView.alignment == .left ? s.width + 40 : bounds.width - s.width - 40
    if point.x > orientationChangeX {
      pointInfoView.alignment = .left
      pointInfoView.center = CGPoint(x: point.x - s.width / 2 - 20, y: pointInfoView.center.y)
    } else {
      pointInfoView.alignment = .right
      pointInfoView.center = CGPoint(x: point.x + s.width / 2 + 20, y: pointInfoView.center.y)
    }
    var f = pointInfoView.frame
    if f.minX < 0 {
      f.origin.x = 0
      pointInfoView.frame = f
    } else if f.minX + f.width > bounds.width {
      f.origin.x = bounds.width - f.width
      pointInfoView.frame = f
    }
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    var pf = pointsView.frame
    pf.origin.y = bounds.minY
    pf.size.height = bounds.height
    pointsView.frame = pf

    var mf = myPositionView.frame
    mf.origin.y = bounds.minY
    mf.size.height = bounds.height
    myPositionView.frame = mf

    if lineInfo == nil, bounds.width > 0 {
      let x = bounds.width * infoX
      guard let (date, intersectionPoints) = delegate?.chartInfoView(self, infoAtPointX: x) else { return }
      lineInfo = intersectionPoints[0]
      pointsView.setPoints(intersectionPoints)
      pointInfoView.set(x: x, label: date, points: intersectionPoints)
      updateViews(point: lineInfo!.point)
    }
  }
}

extension ChartInfoView: UIGestureRecognizerDelegate {
  func gestureRecognizer(_ gestureRecognizer: UIGestureRecognizer,
                         shouldRecognizeSimultaneouslyWith otherGestureRecognizer: UIGestureRecognizer) -> Bool {
    return !captured
  }
}
