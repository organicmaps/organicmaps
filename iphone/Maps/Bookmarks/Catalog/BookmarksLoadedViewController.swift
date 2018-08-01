class BookmarksLoadedViewController: UIViewController {
  private let transitioning = AlertTransitioning()
  @objc var onViewBlock: MWMVoidBlock?
  @objc var onCancelBlock: MWMVoidBlock?

  @IBAction func onViewMap(_ sender: UIButton) {
    onViewBlock?()
  }

  @IBAction func onNotNow(_ sender: UIButton) {
    onCancelBlock?()
  }

  override var transitioningDelegate: UIViewControllerTransitioningDelegate? {
    get { return transitioning }
    set { }
  }

  override var modalPresentationStyle: UIModalPresentationStyle {
    get { return .custom }
    set { }
  }
}
