class AvailableArea: UIView {
  private let observeKeyPath = "sublayers"

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
    ol.addObserver(self, forKeyPath: observeKeyPath, options: .new, context: nil)
  }

  private func unsubscribe() {
    guard let ol = superview?.layer else { return }
    ol.removeObserver(self, forKeyPath: observeKeyPath)
  }

  override func observeValue(forKeyPath keyPath: String?, of _: Any?, change _: [NSKeyValueChangeKey: Any]?, context _: UnsafeMutableRawPointer?) {
    if keyPath == observeKeyPath {
      DispatchQueue.main.async {
        self.update()
      }
    }
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    notifyObserver(frame)
  }

  private func update() {
    guard let sv = superview else { return }
    let newAVs = Set<UIView>(sv.subviews.filter(filterAffectingViews))
    newAVs.subtracting(affectingViews).forEach(addAffectingView)
    affectingViews = newAVs
  }

  func addConstraints(otherView: UIView, directions: MWMAvailableAreaAffectDirections) {
    precondition(!directions.isEmpty)
    let add = { (sa: NSLayoutAttribute, oa: NSLayoutAttribute, rel: NSLayoutRelation) in
      NSLayoutConstraint(item: self, attribute: sa, relatedBy: rel, toItem: otherView, attribute: oa, multiplier: 1, constant: 0).isActive = true
    }
    [.top : (.top, .bottom, .greaterThanOrEqual),
     .bottom : (.bottom, .top, .lessThanOrEqual),
     .left : (.left, .right, .greaterThanOrEqual),
     .right : (.right, .left, .lessThanOrEqual)
      ]
      .filter { directions.contains($0.key) }
      .map { $0.value }
      .forEach(add)
  }

  func filterAffectingViews(_ other: UIView) -> Bool {
    return false
  }

  func addAffectingView(_ other: UIView) {
  }

  func notifyObserver(_ rect: CGRect) {
  }
}

extension MWMAvailableAreaAffectDirections: Hashable {
  public var hashValue: Int {
    return rawValue
  }
}
