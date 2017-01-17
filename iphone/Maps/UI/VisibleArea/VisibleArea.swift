import UIKit

class VisibleArea: UIView {

  @objc(MWMVisibleAreaAffectDirection)
  enum Direction: Int {

    case none
    case top
    case bottom
    case left
    case right
  }

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

  private func update() {
    guard let sv = superview else { return }
    let newAVs = Set<UIView>(sv.subviews.filter { $0.visibleAreaAffectDirection != .none })
    newAVs.subtracting(affectingViews).forEach { addAffectingView($0.visibleAreaAffectView) }
    affectingViews = newAVs
  }

  private func addAffectingView(_ other: UIView?) {
    guard let ov = other else { return }
    let sa: NSLayoutAttribute
    let oa: NSLayoutAttribute
    let rel: NSLayoutRelation
    switch ov.visibleAreaAffectDirection {
    case .none: return
    case .top: (sa, oa, rel) = (.top, .bottom, .greaterThanOrEqual)
    case .bottom: (sa, oa, rel) = (.bottom, .top, .lessThanOrEqual)
    case .left: (sa, oa, rel) = (.left, .right, .greaterThanOrEqual)
    case .right: (sa, oa, rel) = (.right, .left, .lessThanOrEqual)
    }
    NSLayoutConstraint(item: self, attribute: sa, relatedBy: rel, toItem: ov, attribute: oa, multiplier: 1, constant: 0).isActive = true
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    MWMFrameworkHelper.setVisibleViewport(frame)
  }
}

extension UIView {

  var visibleAreaAffectDirection: VisibleArea.Direction { return .none }

  var visibleAreaAffectView: UIView { return self }
}
