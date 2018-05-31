import SafariServices

class TermsOfUseController: WelcomeViewController {

  private enum UserDefaultsKeys {
    static let needTermsKey = "TermsOfUseController_needTerms"
    static let ppLinkKey = "TermsOfUseController_ppLink"
    static let tosLinkKey = "TermsOfUseController_tosLink"
    static let acceptTimeKey = "TermsOfUseController_acceptTime"
  }

  static var needTerms: Bool {
    get {
      return UserDefaults.standard.bool(forKey: UserDefaultsKeys.needTermsKey)
    }
    set {
      UserDefaults.standard.set(newValue, forKey: UserDefaultsKeys.needTermsKey)
    }
  }

  static func controller() -> TermsOfUseController {
    let sb = UIStoryboard.instance(.welcome)
    let vc = sb.instantiateViewController(withIdentifier: toString(self)) as! TermsOfUseController
    return vc
  }

  let privacyPolicyLink = MWMAuthorizationViewModel.privacyPolicyLink()
  let termsOfUseLink = MWMAuthorizationViewModel.termsOfUseLink()

  @IBOutlet weak var alertAdditionalText: UILabel!

  @IBOutlet private weak var privacyPolicyTextView: UITextView! {
    didSet {
      let htmlString = String(coreFormat: L("sign_agree_pp_gdpr"), arguments: [privacyPolicyLink])
      let attributes: [NSAttributedStringKey : Any] = [NSAttributedStringKey.font: UIFont.regular16(),
                                                       NSAttributedStringKey.foregroundColor: UIColor.blackPrimaryText()]
      privacyPolicyTextView.attributedText = NSAttributedString.string(withHtml: htmlString,
                                                                       defaultAttributes: attributes)
      privacyPolicyTextView.delegate = self
    }
  }

  @IBOutlet private weak var termsOfUseTextView: UITextView! {
    didSet {
      let htmlString = String(coreFormat: L("sign_agree_tof_gdpr"), arguments: [termsOfUseLink])
      let attributes: [NSAttributedStringKey : Any] = [NSAttributedStringKey.font: UIFont.regular16(),
                                                       NSAttributedStringKey.foregroundColor: UIColor.blackPrimaryText()]
      termsOfUseTextView.attributedText = NSAttributedString.string(withHtml: htmlString,
                                                                    defaultAttributes: attributes)
      termsOfUseTextView.delegate = self
    }
  }

  @IBOutlet private weak var privacyPolicyCheck: Checkmark! {
    didSet {
      privacyPolicyCheck.offTintColor = .blackHintText()
      privacyPolicyCheck.onTintColor = .linkBlue()
    }
  }

  @IBOutlet private weak var termsOfUseCheck: Checkmark! {
    didSet {
      termsOfUseCheck.offTintColor = .blackHintText()
      termsOfUseCheck.onTintColor = .linkBlue()
    }
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    image.image = #imageLiteral(resourceName: "img_welcome")
    alertTitle.text = L("onboarding_welcome_title")
    alertText.text = L("onboarding_welcome_first_subtitle")
    alertAdditionalText.text = L("sign_message_gdpr")
    nextPageButton.setTitle(L("whats_new_next_button"), for: .normal)

    Statistics.logEvent("OnStart_MapsMeConsent_shown")
  }

  @IBAction func onCheck(_ sender: Checkmark) {
    nextPageButton.isEnabled = privacyPolicyCheck.isChecked && termsOfUseCheck.isChecked;
  }

  override func nextPage() {
    TermsOfUseController.needTerms = false
    Statistics.logEvent("OnStart_MapsMeConsent_accepted")
    UserDefaults.standard.set(privacyPolicyLink, forKey: UserDefaultsKeys.ppLinkKey)
    UserDefaults.standard.set(termsOfUseLink, forKey: UserDefaultsKeys.tosLinkKey)
    UserDefaults.standard.set(Date(), forKey: UserDefaultsKeys.acceptTimeKey)
    super.nextPage()
  }
}

extension TermsOfUseController: UITextViewDelegate {
  func textView(_ textView: UITextView, shouldInteractWith URL: URL, in characterRange: NSRange) -> Bool {
    let safari = SFSafariViewController(url: URL)
    present(safari, animated: true, completion: nil)
    return false;
  }
}
