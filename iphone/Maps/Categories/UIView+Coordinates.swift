
extension UIView {
  func center(inContainerView containerView: UIView) -> CGPoint {
    guard let sv = superview else { return .zero }
    var centerPoint = center

    if let scrollView = sv as? UIScrollView, scrollView.zoomScale != 1.0 {
      centerPoint.x += (scrollView.bounds.width - scrollView.contentSize.width) / 2.0 + scrollView.contentOffset.x
      centerPoint.y += (scrollView.bounds.height - scrollView.contentSize.height) / 2.0 + scrollView.contentOffset.y
    }
    return sv.convert(centerPoint, to: containerView)
  }
}
