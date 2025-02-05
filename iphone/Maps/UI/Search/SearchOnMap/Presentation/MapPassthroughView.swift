/// A transparent view that allows touch events to pass through to the MapViewController's view.
///
/// This view is used to enable interaction with the underlying map while still maintaining a
/// transparent overlay. It does not block touch events but forwards them to the specified `passingView`.

final class MapPassthroughView: UIView {
  private weak var passingView: UIView?

  init(passingView: UIView) {
    self.passingView = passingView
    super.init(frame: passingView.bounds)
    self.autoresizingMask = [.flexibleWidth, .flexibleHeight]
    self.alpha = 0
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func hitTest(_ point: CGPoint, with event: UIEvent?) -> UIView? {
    guard let passingView else { return nil }
    let pointInPassthroughView = passingView.convert(point, from: self)
    super.hitTest(point, with: event)
    if passingView.bounds.contains(pointInPassthroughView) {
      return MapViewController.shared()?.view.hitTest(point, with: event)
    }
    return nil
  }
}
