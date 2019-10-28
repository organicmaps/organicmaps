protocol IWelcomeView: class {
  var presenter: IWelcomePresenter?  { get set }

  func configure(config: IWelcomeConfig)
  func setTitle(_ title: String)
  func setText(_ text: String)
  func setNextButtonTitle(_ title: String)
  func setTitleImage(_ titleImage: UIImage?)
  var isCloseButtonHidden: Bool {get set}
}

class WelcomeViewController: MWMViewController {
  var presenter: IWelcomePresenter?

  @IBOutlet private var image: UIImageView!
  @IBOutlet private var alertTitle: UILabel!
  @IBOutlet private var alertText: UILabel!
  @IBOutlet private var nextButton: UIButton!
  @IBOutlet private var closeButton: UIButton!
  @IBOutlet private var closeButtonHeightConstraint: NSLayoutConstraint!

  var isCloseButtonHidden: Bool = false {
    didSet{
      closeButtonHeightConstraint.constant = isCloseButtonHidden ? 0 : 32
    }
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    presenter?.configure()
  }

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)
    presenter?.onAppear()
  }

  @IBAction func onNextButton(_ sender: UIButton) {
    presenter?.onNext()
  }

  @IBAction func onCloseButton(_ sender: UIButton) {
    presenter?.onClose()
  }

  var key: String {
    return presenter?.key() ?? ""
  }
}

extension WelcomeViewController: IWelcomeView {
  func configure(config: IWelcomeConfig) {
    setTitle(L(config.title))
    setText(L(config.text))
    setTitleImage(config.image)
    setNextButtonTitle(L(config.buttonNextTitle))
    isCloseButtonHidden = config.isCloseButtonHidden
  }

  func setTitle(_ title: String) {
    alertTitle.text = title;
  }

  func setText(_ text: String) {
    alertText.text = text;
  }

  func setNextButtonTitle(_ title: String) {
    nextButton.setTitle(title, for: .normal)
  }

  func setTitleImage(_ titleImage: UIImage?) {
    image.image = titleImage
  }
}

