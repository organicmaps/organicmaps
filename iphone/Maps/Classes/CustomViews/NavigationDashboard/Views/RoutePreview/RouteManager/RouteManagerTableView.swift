final class RouteManagerTableView: UITableView {
  @IBOutlet private weak var tableViewHeight: NSLayoutConstraint!

  enum HeightUpdateStyle {
    case animated
    case deferred
    case off
  }

  var heightUpdateStyle = HeightUpdateStyle.deferred

  override var contentSize: CGSize {
    didSet {
      guard contentSize != oldValue else { return }
      let sel = #selector(updateHeight)
      NSObject.cancelPreviousPerformRequests(withTarget: self, selector: sel, object: nil)
      switch heightUpdateStyle {
      case .animated:
        guard let sv = superview else { return }
        sv.setNeedsLayout()
        updateHeight()
        UIView.animate(withDuration: kDefaultAnimationDuration) { sv.layoutIfNeeded() }
      case .deferred: perform(sel, with: nil, afterDelay: 0)
      case .off: break
      }
    }
  }

  @objc
  private func updateHeight() {
    tableViewHeight.constant = contentSize.height
  }
}
