import SafariServices

protocol ITermsOfUseView: class {
  var presenter: ITermsOfUsePresenter?  { get set }

  func setTitle(_ title: String)
  func setText(_ text: String)
  func setTitleImage(_ titleImage: UIImage?)
  func setPrivacyPolicyTitle( _ htmlString: String)
  func setTermsOfUseTitle( _ htmlString: String)
}

class TermsOfUseViewController: MWMViewController {
  var presenter: ITermsOfUsePresenter?

  @IBOutlet private var image: UIImageView!
  @IBOutlet private var alertTitle: UILabel!
  @IBOutlet private var alertText: UILabel!

  @IBOutlet private weak var privacyPolicyTextView: UITextView! {
    didSet {
      privacyPolicyTextView.delegate = self
    }
  }

  @IBOutlet private weak var termsOfUseTextView: UITextView! {
    didSet {
      termsOfUseTextView.delegate = self
    }
  }

  @IBOutlet private weak var privacyPolicyCheck: Checkmark!
  @IBOutlet private weak var termsOfUseCheck: Checkmark!

  override func viewDidLoad() {
    super.viewDidLoad()
    presenter?.configure()
  }

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)
    presenter?.onAppear()
  }

  @IBAction func onCheck(_ sender: Checkmark) {
    if (privacyPolicyCheck.isChecked && termsOfUseCheck.isChecked) {
      presenter?.onNext()
    }
  }

  @IBAction func onPrivacyTap(_ sender: UITapGestureRecognizer) {
    privacyPolicyCheck.isChecked = !privacyPolicyCheck.isChecked
    onCheck(privacyPolicyCheck)
  }

  @IBAction func onTermsTap(_ sender: UITapGestureRecognizer) {
    termsOfUseCheck.isChecked = !termsOfUseCheck.isChecked
    onCheck(termsOfUseCheck)
  }
}

extension TermsOfUseViewController: ITermsOfUseView {
  func setTitle(_ title: String) {
    alertTitle.text = title
  }

  func setText(_ text: String) {
    alertText.text = text
  }

  func setTitleImage(_ titleImage: UIImage?) {
    image.image = titleImage
  }

  func setPrivacyPolicyTitle(_ htmlString: String) {
    setHtmlTitle(textView: privacyPolicyTextView, htmlString: htmlString)
  }

  func setTermsOfUseTitle(_ htmlString: String) {
    setHtmlTitle(textView: termsOfUseTextView, htmlString: htmlString)
  }

  private func setHtmlTitle(textView: UITextView, htmlString: String) {
    textView.attributedText = NSAttributedString.string(withHtml: htmlString,
                                                        defaultAttributes: [:])
  }
}

extension TermsOfUseViewController: UITextViewDelegate {
  func textView(_ textView: UITextView, shouldInteractWith URL: URL, in characterRange: NSRange) -> Bool {
    let safari = SFSafariViewController(url: URL)
    present(safari, animated: true, completion: nil)
    return false;
  }
}
