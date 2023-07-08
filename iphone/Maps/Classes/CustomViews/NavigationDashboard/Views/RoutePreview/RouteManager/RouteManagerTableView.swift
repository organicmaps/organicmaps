final class RouteManagerTableView: UITableView {
  @IBOutlet private weak var tableViewHeight: NSLayoutConstraint!

  enum HeightUpdateStyle {
    case animated
    case deferred
    case off
  }

  var heightUpdateStyle = HeightUpdateStyle.deferred

  private var scheduledUpdate: DispatchWorkItem?

  override var contentSize: CGSize {
    didSet {
      guard contentSize != oldValue else { return }
      scheduledUpdate?.cancel()
      let update = { [weak self] in
        guard let s = self else { return }
        s.tableViewHeight.constant = s.contentSize.height
      }
      switch heightUpdateStyle {
      case .animated: superview?.animateConstraints(animations: update)
      case .deferred:
        scheduledUpdate = DispatchWorkItem(block: update)
        DispatchQueue.main.async(execute: scheduledUpdate!)
      case .off: break
      }
    }
  }
}
