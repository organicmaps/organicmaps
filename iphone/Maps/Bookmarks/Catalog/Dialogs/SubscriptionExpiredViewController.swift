class SubscriptionExpiredViewController: UIViewController {
  private let transitioning = FadeTransitioning<AlertPresentationController>(cancellable: false)
  private let onSubscribe: MWMVoidBlock
  private let onDelete: MWMVoidBlock

  init(onSubscribe: @escaping MWMVoidBlock, onDelete: @escaping MWMVoidBlock) {
    self.onSubscribe = onSubscribe
    self.onDelete = onDelete
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

  @IBAction func onSubscribe(_ sender: UIButton) {
    onSubscribe()
  }

  @IBAction func onDelete(_ sender: UIButton) {
    onDelete()
  }
}
