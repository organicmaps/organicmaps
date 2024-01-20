class AvailableArea: UIView {
  private enum Const {
    static let observeKeyPath = "sublayers"
  }

  var deferNotification: Bool { return true }

  private(set) var orientation = UIDeviceOrientation.unknown {
    didSet {
      scheduleNotification()
    }
  }

  var shouldUpdateAreaFrame: Bool {
    if let insets = UIApplication.shared.delegate?.window??.safeAreaInsets {
      return insets.left > 0 || insets.right > 0
    } else {
      return false
    }
  }

  var areaFrame: CGRect {
    return alternative(iPhone: {
      var frame = self.frame
      if self.shouldUpdateAreaFrame {
        switch self.orientation {
        case .landscapeLeft:
          frame.origin.x -= 16
          frame.size.width += 60
        case .landscapeRight:
          frame.origin.x -= 44
          frame.size.width += 60
        default: break
        }
      }
      return frame
    }, iPad: { self.frame })()
  }

  private var affectingViews = Set<UIView>()

  override func didMoveToSuperview() {
    super.didMoveToSuperview()
    subscribe()
    update()
  }

  deinit {
    unsubscribe()
  }

  private func subscribe() {
    guard let ol = superview?.layer else { return }
    ol.addObserver(self, forKeyPath: Const.observeKeyPath, options: .new, context: nil)
    UIDevice.current.beginGeneratingDeviceOrientationNotifications()

    let nc = NotificationCenter.default
    nc.addObserver(forName: UIDevice.orientationDidChangeNotification, object: nil, queue: .main) { _ in
      let orientation = UIDevice.current.orientation
      guard !orientation.isFlat && orientation != .portraitUpsideDown else { return }
      self.orientation = orientation
    }
  }

  private func unsubscribe() {
    guard let ol = superview?.layer else { return }
    ol.removeObserver(self, forKeyPath: Const.observeKeyPath)
    UIDevice.current.endGeneratingDeviceOrientationNotifications()
  }

  override func observeValue(forKeyPath keyPath: String?, of _: Any?, change _: [NSKeyValueChangeKey: Any]?, context _: UnsafeMutableRawPointer?) {
    if keyPath == Const.observeKeyPath {
      DispatchQueue.main.async {
        self.update()
      }
    }
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    scheduleNotification()
  }

  private func newAffectingViews(view: UIView) -> Set<UIView> {
    var views = Set<UIView>()
    if isAreaAffectingView(view) {
      views.insert(view)
    }
    view.subviews.forEach {
      views.formUnion(newAffectingViews(view: $0))
    }
    return views
  }

  private func update() {
    guard let sv = superview else { return }
    let newAVs = newAffectingViews(view: sv)
    newAVs.subtracting(affectingViews).forEach(addAffectingView)
    affectingViews = newAVs
    scheduleNotification()
  }

  func addConstraints(otherView: UIView, directions: MWMAvailableAreaAffectDirections) {
    precondition(!directions.isEmpty)
    let add = { (sa: NSLayoutConstraint.Attribute, oa: NSLayoutConstraint.Attribute, rel: NSLayoutConstraint.Relation) in
      let c = NSLayoutConstraint(item: self, attribute: sa, relatedBy: rel, toItem: otherView, attribute: oa, multiplier: 1, constant: 0)
      c.priority = UILayoutPriority.defaultHigh
      c.isActive = true
    }
    [
      .top: (.top, .bottom, .greaterThanOrEqual),
      .bottom: (.bottom, .top, .lessThanOrEqual),
      .left: (.left, .right, .greaterThanOrEqual),
      .right: (.right, .left, .lessThanOrEqual),
    ]
    .filter { directions.contains($0.key) }
    .map { $0.value }
    .forEach(add)
  }

  @objc
  private func scheduleNotification() {
    if deferNotification {
      let selector = #selector(notifyObserver)
      NSObject.cancelPreviousPerformRequests(withTarget: self, selector: selector, object: nil)
      perform(selector, with: nil, afterDelay: 0)
    } else {
      notifyObserver()
    }
  }

  func isAreaAffectingView(_: UIView) -> Bool { return false }
  func addAffectingView(_: UIView) {}
  @objc func notifyObserver() {}
}

extension MWMAvailableAreaAffectDirections: Hashable {
  public var hashValue: Int {
    return rawValue
  }
}
