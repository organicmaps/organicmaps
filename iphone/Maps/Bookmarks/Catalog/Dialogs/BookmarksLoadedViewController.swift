class BookmarksLoadedViewController: UIViewController {
  private let transitioning = FadeTransitioning<AlertPresentationController>()
  @objc var onViewBlock: MWMVoidBlock?
  @objc var onCancelBlock: MWMVoidBlock?

  override init(nibName nibNameOrNil: String?, bundle nibBundleOrNil: Bundle?) {
    super.init(nibName: nibNameOrNil, bundle: nibBundleOrNil)
    transitioningDelegate = transitioning
    modalPresentationStyle = .custom
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  @IBAction func onViewMap(_ sender: UIButton) {
    onViewBlock?()
  }

  @IBAction func onNotNow(_ sender: UIButton) {
    onCancelBlock?()
  }
}
