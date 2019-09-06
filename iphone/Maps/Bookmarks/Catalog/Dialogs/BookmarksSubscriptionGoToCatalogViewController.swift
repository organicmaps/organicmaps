@objc class BookmarksSubscriptionGoToCatalogViewController: UIViewController {
  private let transitioning = FadeTransitioning<AlertPresentationController>()
  private let onOk: MWMVoidBlock
  private let onCancel: MWMVoidBlock

  @objc init(onOk: @escaping MWMVoidBlock, onCancel: @escaping MWMVoidBlock) {
    self.onOk = onOk
    self.onCancel = onCancel
    super.init(nibName: nil, bundle: nil)
    transitioningDelegate = transitioning
    modalPresentationStyle = .custom
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  @IBAction func onOk(_ sender: UIButton) {
    onOk()
  }

  @IBAction func onCancel(_ sender: UIButton) {
    onCancel()
  }
}
