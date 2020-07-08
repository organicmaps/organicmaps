class SubscriptionSuccessViewController: UIViewController {
  private let transitioning = FadeTransitioning<AlertPresentationController>()
  private let onOk: MWMVoidBlock
  private let screenType: SubscriptionGroupType

  @IBOutlet private var titleLabel: UILabel!
  @IBOutlet private var textLabel: UILabel!

  init(_ screenType: SubscriptionGroupType, onOk: @escaping MWMVoidBlock) {
    self.onOk = onOk
    self.screenType = screenType
    super.init(nibName: nil, bundle: nil)
    transitioningDelegate = transitioning
    modalPresentationStyle = .custom
  }

  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func awakeFromNib() {
    super.awakeFromNib()
    switch screenType {
    case .city:
      titleLabel.text = L("subscription_success_dialog_title_sightseeing_pass")
      textLabel.text = L("subscription_success_dialog_message_sightseeing_pass")
    case .allPass:
      titleLabel.text = L("subscription_success_dialog_title")
      textLabel.text = L("subscription_success_dialog_message")
    }
  }

  override func viewDidLoad() {
    super.viewDidLoad()
  }

  @IBAction func onOk(_ sender: UIButton) {
    onOk()
  }
}
