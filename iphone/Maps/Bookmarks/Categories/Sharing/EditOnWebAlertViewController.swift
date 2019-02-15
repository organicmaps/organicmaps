class EditOnWebAlertViewController: UIViewController {
  private let transitioning = FadeTransitioning<AlertPresentationController>()
  private var alertTitle = ""
  private var alertMessage = ""
  private var buttonTitle = ""

  var onAcceptBlock: MWMVoidBlock?
  var onCancelBlock: MWMVoidBlock?

  @IBOutlet weak var titleLabel: UILabel!
  @IBOutlet weak var messageLabel: UILabel!
  @IBOutlet weak var cancelButton: UIButton!
  @IBOutlet weak var acceptButton: UIButton!

  convenience init(with title: String, message: String, acceptButtonTitle: String) {
    self.init()
    alertTitle = title
    alertMessage = message
    buttonTitle = acceptButtonTitle
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    titleLabel.text = alertTitle
    messageLabel.text = alertMessage
    acceptButton.setTitle(buttonTitle, for: .normal)
    cancelButton.setTitle(L("cancel"), for: .normal)
  }

  override var transitioningDelegate: UIViewControllerTransitioningDelegate? {
    get { return transitioning }
    set { }
  }

  override var modalPresentationStyle: UIModalPresentationStyle {
    get { return .custom }
    set { }
  }

  @IBAction func onAccept(_ sender: UIButton) {
    onAcceptBlock?()
  }

  @IBAction func onCancel(_ sender: UIButton) {
    onCancelBlock?()
  }
}
