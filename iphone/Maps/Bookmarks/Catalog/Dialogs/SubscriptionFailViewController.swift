class SubscriptionFailViewController: UIViewController {
  private let transitioning = FadeTransitioning<AlertPresentationController>()
  private let onOk: MWMVoidBlock

  init(onOk: @escaping MWMVoidBlock) {
    self.onOk = onOk
    super.init(nibName: nil, bundle: nil)
    transitioningDelegate = transitioning
    modalPresentationStyle = .custom
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()
  }

  @IBAction func onOk(_ sender: UIButton) {
    onOk()
  }
}
