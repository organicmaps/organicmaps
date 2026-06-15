import UIKit

struct ChartLineInfo {
  let color: UIColor
  let point: CGPoint
  let formattedValue: String
}

protocol ChartInfoViewDelegate: AnyObject {
  func chartInfoView(_ view: ChartInfoView, infoAtPointX pointX: CGFloat) -> (String, [ChartLineInfo])?
  func chartInfoView(_ view: ChartInfoView, didMoveToPoint pointX: CGFloat)
  func chartInfoView(_ view: ChartInfoView, shouldStartSelectingAtPoint pointX: CGFloat) -> Bool
}

class ChartInfoView: ExpandedTouchView {
  weak var delegate: ChartInfoViewDelegate?

  private let pointInfoView = ChartPointInfoView()
  private let pointsView = ChartPointIntersectionView(frame: CGRect(x: 0, y: 0, width: 2, height: 0))
  private let myPositionView = ChartMyPositionView(frame: CGRect(x: 0, y: 0, width: 2, height: 0))
  private var lineInfo: ChartLineInfo?

  fileprivate var captured = false

  private var _infoX: CGFloat = 0
  var infoX: CGFloat {
    get { _infoX }
    set {
      _infoX = newValue
      update(bounds.width * _infoX)
    }
  }

  var myPositionX: CGFloat = -1 {
    didSet {
      if myPositionX < 0 || myPositionX > 1 {
        myPositionView.isHidden = true
        return
      }
      myPositionView.isHidden = false
      updateMyPosition()
    }
  }

  var tooltipBackgroundColor: UIColor = .white {
    didSet {
      pointInfoView.backgroundColor = tooltipBackgroundColor
    }
  }

  var font: UIFont = .systemFont(ofSize: 12, weight: .regular) {
    didSet {
      pointInfoView.font = font
    }
  }

  var textColor: UIColor = .black {
    didSet {
      pointInfoView.textColor = textColor
    }
  }

  var infoBackgroundColor: UIColor = .white {
    didSet {
      pointInfoView.backgroundColor = infoBackgroundColor
    }
  }

  var infoShadowColor: UIColor = .black {
    didSet {
      updateColors()
    }
  }

  var infoShadowOpacity: Float = 0.25 {
    didSet {
      pointInfoView.layer.shadowOpacity = infoShadowOpacity
    }
  }

  private var scrubGR: UILongPressGestureRecognizer!
  var selectionGestureRecognizer: UIGestureRecognizer { scrubGR }

  override init(frame: CGRect) {
    super.init(frame: frame)

    addSubview(myPositionView)
    myPositionView.isHidden = true
    addSubview(pointsView)
    addSubview(pointInfoView)
    isExclusiveTouch = true
    scrubGR = UILongPressGestureRecognizer(target: self, action: #selector(onScrub(_:)))
    scrubGR.minimumPressDuration = 0
    scrubGR.allowableMovement = .greatestFiniteMagnitude
    scrubGR.delegate = self
    addGestureRecognizer(scrubGR)
    pointInfoView.textColor = textColor
    pointInfoView.backgroundColor = tooltipBackgroundColor
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError()
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    guard traitCollection.hasDifferentColorAppearance(comparedTo: previousTraitCollection) else { return }
    updateColors()
  }

  private func updateColors() {
    pointInfoView.layer.shadowColor = infoShadowColor.resolvedColor(with: traitCollection).cgColor
  }

  func update(_ x: CGFloat? = nil) {
    guard bounds.width > 0 else { return }
    let x = x ?? pointsView.center.x
    _infoX = x / bounds.width
    guard let delegate = delegate,
          let (label, intersectionPoints) = delegate.chartInfoView(self, infoAtPointX: x) else { return }
    lineInfo = intersectionPoints[0]
    pointsView.updatePoint(lineInfo!)
    pointInfoView.update(x: x, label: label, points: intersectionPoints)
    updateViews(point: lineInfo!.point)
  }

  private func updateMyPosition() {
    myPositionView.center = CGPoint(x: bounds.width * myPositionX, y: myPositionView.center.y)
    guard let (_, myPositionPoints) = delegate?.chartInfoView(self, infoAtPointX: myPositionView.center.x) else { return }
    myPositionView.pinY = myPositionPoints[0].point.y
  }

  private func shouldCapture(at pointX: CGFloat) -> Bool {
    delegate?.chartInfoView(self, shouldStartSelectingAtPoint: pointX) ?? false
  }

  @objc private func onScrub(_ sender: UILongPressGestureRecognizer) {
    let x = max(bounds.minX, min(bounds.maxX, sender.location(in: self).x))
    switch sender.state {
    case .possible:
      break
    case .began:
      captured = true
      update(x)
      delegate?.chartInfoView(self, didMoveToPoint: x)
    case .changed:
      guard captured else { return }
      update(x)
      delegate?.chartInfoView(self, didMoveToPoint: x)
    case .ended, .cancelled, .failed:
      captured = false
    @unknown default:
      fatalError()
    }
  }

  private func updateViews(point: CGPoint) {
    pointsView.alpha = 1
    pointsView.center = CGPoint(x: point.x, y: bounds.midY)

    let s = pointInfoView.systemLayoutSizeFitting(UIView.layoutFittingCompressedSize)
    pointInfoView.frame.size = s
    let y = max(pointInfoView.frame.height / 2 + 5,
                min(bounds.height - pointInfoView.frame.height / 2 - 5, bounds.height - lineInfo!.point.y))
    let orientationChangeX = pointInfoView.alignment == .left ? s.width + 40 : bounds.width - s.width - 40
    if point.x > orientationChangeX {
      pointInfoView.alignment = .left
      pointInfoView.center = CGPoint(x: point.x - s.width / 2 - 20, y: y)
    } else {
      pointInfoView.alignment = .right
      pointInfoView.center = CGPoint(x: point.x + s.width / 2 + 20, y: y)
    }
    var f = pointInfoView.frame
    if f.minX < 0 {
      f.origin.x = 0
      pointInfoView.frame = f
    } else if f.minX + f.width > bounds.width {
      f.origin.x = bounds.width - f.width
      pointInfoView.frame = f
    }
    let arrowPoint = convert(CGPoint(x: 0, y: bounds.height - lineInfo!.point.y), to: pointInfoView)
    pointInfoView.arrowY = arrowPoint.y
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

    update(bounds.width * infoX)
    updateMyPosition()
  }
}

extension ChartInfoView: UIGestureRecognizerDelegate {
  override func gestureRecognizerShouldBegin(_ gestureRecognizer: UIGestureRecognizer) -> Bool {
    guard gestureRecognizer === scrubGR else { return true }
    let pointX = gestureRecognizer.location(in: self).x
    return shouldCapture(at: pointX)
  }

  func gestureRecognizer(_ gestureRecognizer: UIGestureRecognizer,
                         shouldRecognizeSimultaneouslyWith otherGestureRecognizer: UIGestureRecognizer) -> Bool {
    if otherGestureRecognizer is UIPinchGestureRecognizer {
      return true
    }

    if captured {
      return false
    }

    let pointX = gestureRecognizer.location(in: self).x
    let shouldCapture = shouldCapture(at: pointX)
    return !shouldCapture
  }
}
