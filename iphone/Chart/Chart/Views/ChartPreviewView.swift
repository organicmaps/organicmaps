import UIKit

protocol ChartPreviewViewDelegate: AnyObject {
  func chartPreviewView(_ view: ChartPreviewView, didChangeMinX minX: Int, maxX: Int)
}

class TintView: UIView {
  let maskLayer = CAShapeLayer()

  override init(frame: CGRect = .zero) {
    super.init(frame: frame)
    maskLayer.fillRule = .evenOdd
    layer.mask = maskLayer
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError()
  }

  func updateViewport(_ viewport: CGRect) {
    let cornersMask = UIBezierPath(roundedRect: bounds, cornerRadius: 5)
    let rectMask = UIBezierPath(rect: viewport.insetBy(dx: 11, dy: 1))
    let result = UIBezierPath()
    result.append(cornersMask)
    result.append(rectMask)
    result.usesEvenOddFillRule = true
    maskLayer.path = result.cgPath
  }
}

class ViewPortView: ExpandedTouchView {
  let maskLayer = CAShapeLayer()
  var tintView: TintView?

  override init(frame: CGRect = .zero) {
    super.init(frame: frame)
    maskLayer.fillRule = .evenOdd
    layer.mask = maskLayer
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError()
  }

  override var frame: CGRect {
    didSet {
      maskLayer.path = makeMaskPath().cgPath
      tintView?.updateViewport(convert(bounds, to: tintView))
    }
  }

  func makeMaskPath() -> UIBezierPath {
    let cornersMask = UIBezierPath(roundedRect: bounds, cornerRadius: 5)
    let rectMask = UIBezierPath(rect: bounds.insetBy(dx: 11, dy: 1))
    let result = UIBezierPath()
    result.append(cornersMask)
    result.append(rectMask)
    result.usesEvenOddFillRule = true
    return result
  }
}

class ChartPreviewView: ExpandedTouchView {
  let previewContainerView = UIView()
  let viewPortView = ViewPortView()
  let leftBoundView = UIView()
  let rightBoundView = UIView()
  let tintView = TintView()
  var previewViews: [ChartLineView] = []

  var selectorColor: UIColor = UIColor.white {
    didSet {
      viewPortView.backgroundColor = selectorColor
    }
  }

  var selectorTintColor: UIColor = UIColor.clear {
    didSet {
      tintView.backgroundColor = selectorTintColor
    }
  }

  var minX = 0
  var maxX = 0
  weak var delegate: ChartPreviewViewDelegate?

  override var frame: CGRect {
    didSet {
      if chartData != nil {
        updateViewPort()
      }
    }
  }

  var chartData: ChartPresentationData! {
    didSet {
      previewViews.forEach { $0.removeFromSuperview() }
      previewViews.removeAll()
      for i in (0..<chartData.linesCount).reversed() {
        let line = chartData.lineAt(i)
        let v = ChartLineView()
        v.isPreview = true
        v.chartLine = line
        v.frame = previewContainerView.bounds
        v.autoresizingMask = [.flexibleWidth, .flexibleHeight]
        previewContainerView.addSubview(v)
        previewViews.insert(v, at: 0)
      }
      previewViews.forEach { $0.setY(min: chartData.lower, max: chartData.upper) }
      let count = chartData.pointsCount - 1
      minX = 0
      maxX = count
      updateViewPort()
    }
  }

  override init(frame: CGRect) {
    super.init(frame: frame)
    previewContainerView.translatesAutoresizingMaskIntoConstraints = false
    previewContainerView.layer.cornerRadius = 5
    previewContainerView.clipsToBounds = true
    addSubview(previewContainerView)
    let t = previewContainerView.topAnchor.constraint(equalTo: topAnchor)
    let b = previewContainerView.bottomAnchor.constraint(equalTo: bottomAnchor)
    t.priority = .defaultHigh
    b.priority = .defaultHigh
    t.constant = 1
    b.constant = -1
    NSLayoutConstraint.activate([
      previewContainerView.leftAnchor.constraint(equalTo: leftAnchor),
      previewContainerView.rightAnchor.constraint(equalTo: rightAnchor),
      t,
      b])

    tintView.frame = bounds
    tintView.backgroundColor = selectorTintColor
    tintView.autoresizingMask = [.flexibleWidth, .flexibleHeight]
    addSubview(tintView)

    viewPortView.tintView = tintView
    viewPortView.backgroundColor = selectorColor
    viewPortView.translatesAutoresizingMaskIntoConstraints = false
    addSubview(viewPortView)

    viewPortView.addSubview(leftBoundView)
    viewPortView.addSubview(rightBoundView)

    let pan = UIPanGestureRecognizer(target: self, action: #selector(onPan(_:)))
    viewPortView.addGestureRecognizer(pan)

    let leftPan = UIPanGestureRecognizer(target: self, action: #selector(onLeftPan(_:)))
    let rightPan = UIPanGestureRecognizer(target: self, action: #selector(onRightPan(_:)))
    leftBoundView.addGestureRecognizer(leftPan)
    rightBoundView.addGestureRecognizer(rightPan)
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError()
  }

  @objc func onPan(_ sender: UIPanGestureRecognizer) {
    if sender.state != .changed { return }

    let p = sender.translation(in: viewPortView)
    let count = chartData.labels.count - 1
    let x = Int((viewPortView.frame.minX + p.x) / bounds.width * CGFloat(count))
    let dx = maxX - minX
    let mx = x + dx

    if x > 0 && mx < count {
      viewPortView.frame = viewPortView.frame.offsetBy(dx: p.x, dy: 0)
      sender.setTranslation(CGPoint(x: 0, y: 0), in: viewPortView)
      if x != minX {
        minX = x
        maxX = mx
        delegate?.chartPreviewView(self, didChangeMinX: minX, maxX: maxX)
      }
    } else if minX > 0 && x <= 0 {
      setX(min: 0, max: dx)
    } else if maxX < count && mx >= count {
      setX(min: count - dx, max: count)
    }
  }

  @objc func onLeftPan(_ sender: UIPanGestureRecognizer) {
    if sender.state != .changed { return }

    let p = sender.translation(in: leftBoundView)
    let count = chartData.labels.count - 1
    let x = Int((viewPortView.frame.minX + p.x) / bounds.width * CGFloat(count))

    if x > 0 && x < maxX && maxX - x >= count / 10 {
      var f = viewPortView.frame
      f = CGRect(x: f.minX + p.x, y: f.minY, width: f.width - p.x, height: f.height)
      viewPortView.frame = f
      rightBoundView.frame = CGRect(x: viewPortView.bounds.width - 14, y: 0, width: 44, height: viewPortView.bounds.height)
      sender.setTranslation(CGPoint(x: 0, y: 0), in: leftBoundView)
      if x != minX {
        minX = x
        delegate?.chartPreviewView(self, didChangeMinX: minX, maxX: maxX)
      }
    } else if x <= 0 && minX > 0 {
      setX(min: 0, max: maxX)
    }
  }

  @objc func onRightPan(_ sender: UIPanGestureRecognizer) {
    if sender.state != .changed { return }

    let p = sender.translation(in: viewPortView)
    let count = chartData.labels.count - 1
    let x = Int((viewPortView.frame.maxX + p.x) / bounds.width * CGFloat(count))

    if x > minX && x < count && x - minX >= count / 10 {
      var f = viewPortView.frame
      f = CGRect(x: f.minX, y: f.minY, width: f.width + p.x, height: f.height)
      viewPortView.frame = f
      rightBoundView.frame = CGRect(x: viewPortView.bounds.width - 14, y: 0, width: 44, height: viewPortView.bounds.height)
      sender.setTranslation(CGPoint(x: 0, y: 0), in: rightBoundView)
      if x != maxX {
        maxX = x
        delegate?.chartPreviewView(self, didChangeMinX: minX, maxX: maxX)
      }
    } else if x >= count && maxX < count {
      setX(min: minX, max: count)
    }
  }

  func setX(min: Int, max: Int) {
    guard min < max else { return }
    minX = min
    maxX = max
    updateViewPort()
    delegate?.chartPreviewView(self, didChangeMinX: minX, maxX: maxX)
  }

  func updateViewPort() {
    let count = CGFloat(chartData.labels.count - 1)
    viewPortView.frame = CGRect(x: CGFloat(minX) / count * bounds.width,
                                y: bounds.minY,
                                width: CGFloat(maxX - minX) / count * bounds.width,
                                height: bounds.height)
    leftBoundView.frame = CGRect(x: -30, y: 0, width: 44, height: viewPortView.bounds.height)
    rightBoundView.frame = CGRect(x: viewPortView.bounds.width - 14, y: 0, width: 44, height: viewPortView.bounds.height)
  }
}
