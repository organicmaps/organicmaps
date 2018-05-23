import FBSDKCoreKit
import FBSDKLoginKit
import GoogleSignIn
import SafariServices

@objc(MWMAuthorizationViewController)
final class AuthorizationViewController: MWMViewController {
  typealias ViewModel = MWMAuthorizationViewModel

  private let transitioningManager: AuthorizationTransitioningManager

  lazy var chromeView: UIView = {
    let view = UIView()
    view.backgroundColor = UIColor.blackStatusBarBackground()
    return view
  }()

  weak var containerView: UIView! {
    didSet {
      containerView.insertSubview(chromeView, at: 0)
    }
  }

  @IBOutlet private weak var contentView: UIView! {
    didSet {
      contentView.backgroundColor = UIColor.white()
    }
  }

  @IBOutlet private weak var titleLabel: UILabel! {
    didSet {
      titleLabel.font = UIFont.bold22()
      titleLabel.textColor = UIColor.blackPrimaryText()
      titleLabel.text = L("profile_authorization_title")
    }
  }

  @IBOutlet weak var separator: UIView! {
    didSet {
      separator.backgroundColor = UIColor.blackDividers()
    }
  }

  @IBOutlet private weak var textLabel: UILabel! {
    didSet {
      textLabel.font = UIFont.regular14()
      textLabel.textColor = UIColor.blackSecondaryText()
      textLabel.text = L("sign_message_gdpr")
    }
  }

  @IBOutlet private weak var googleButton: UIButton! {
    didSet {
      let layer = googleButton.layer
      layer.cornerRadius = 8
      layer.borderWidth = 1
      layer.borderColor = UIColor.blackDividers().cgColor
      googleButton.clipsToBounds = true
      googleButton.setTitle("Google", for: .normal)
      googleButton.setTitleColor(UIColor.blackPrimaryText(), for: .normal)
      googleButton.setTitleColor(UIColor.blackSecondaryText(), for: .disabled)
      googleButton.titleLabel?.font = UIFont.bold14()
      googleButton.isEnabled = false
    }
  }

  @IBAction func googleSignIn() {
    let gid = GIDSignIn.sharedInstance()!
    if var scopes = gid.scopes {
      scopes.append("https://www.googleapis.com/auth/plus.login")
      gid.scopes = scopes
    }
    gid.delegate = self
    gid.uiDelegate = self
    gid.signIn()
  }

  @IBOutlet private weak var facebookButton: UIButton! {
    didSet {
      facebookButton.layer.cornerRadius = 8
      facebookButton.clipsToBounds = true
      facebookButton.titleLabel?.font = UIFont.bold14()
      facebookButton.setBackgroundColor(UIColor.facebookButtonBackground(), for: .normal)
      facebookButton.setBackgroundColor(UIColor.facebookButtonBackgroundDisabled(), for: .disabled)
      facebookButton.isEnabled = false
    }
  }
  
  @IBAction func facebookSignIn() {
    let fbLoginManager = FBSDKLoginManager()
    fbLoginManager.loginBehavior = .systemAccount
    fbLoginManager.logIn(withReadPermissions: ["public_profile", "email"], from: self) { [weak self] (result, error) in
      if let error = error {
        self?.process(error: error, type: .facebook)
      } else if let token = result?.token {
        self?.process(token: token.tokenString, type: .facebook)
      }
    }
  }

  @IBAction private func phoneSignIn() {
    dismiss(animated: true) {
      let url = ViewModel.phoneAuthURL()!
      let wv = WebViewController(authURL: url, onSuccessAuth: { (token: String?) in
        self.process(token: token!, type: .phone)
      }, onFailure: {
        self.process(error: NSError(domain: kMapsmeErrorDomain, code: 0), type: .phone)
        self.errorHandler?(.cancelled)
      })

      MapViewController.topViewController().navigationController?.pushViewController(wv!, animated: true)
    }
  }
  
  @IBOutlet private weak var phoneSignInButton: UIButton! {
    didSet {
      phoneSignInButton.isEnabled = false
    }
  }

  @IBOutlet private weak var privacyPolicyCheck: Checkmark! {
    didSet {
      privacyPolicyCheck.offTintColor = .blackHintText()
      privacyPolicyCheck.onTintColor = .linkBlue()
      privacyPolicyCheck.contentHorizontalAlignment = .left
    }
  }
  
  @IBOutlet private weak var termsOfUseCheck: Checkmark! {
    didSet {
      termsOfUseCheck.offTintColor = .blackHintText()
      termsOfUseCheck.onTintColor = .linkBlue()
      termsOfUseCheck.contentHorizontalAlignment = .left
    }
  }
  
  @IBOutlet private weak var latestNewsCheck: Checkmark! {
    didSet {
      latestNewsCheck.offTintColor = .blackHintText()
      latestNewsCheck.onTintColor = .linkBlue()
      latestNewsCheck.contentHorizontalAlignment = .left
    }
  }
  
  @IBAction func onCheck(_ sender: Checkmark) {
    let allButtonsChecked = privacyPolicyCheck.isChecked &&
      termsOfUseCheck.isChecked;
    
    googleButton.isEnabled = allButtonsChecked;
    facebookButton.isEnabled = allButtonsChecked;
    phoneSignInButton.isEnabled = allButtonsChecked;
  }
  
  @IBOutlet private weak var privacyPolicyTextView: UITextView! {
    didSet {
      let htmlString = L("sign_agree_pp_gdpr")
      let attributes: [NSAttributedStringKey : Any] = [NSAttributedStringKey.font: UIFont.regular16(),
                                                       NSAttributedStringKey.foregroundColor: UIColor.blackPrimaryText()]
      privacyPolicyTextView.attributedText = NSAttributedString.string(withHtml: htmlString,
                                                                       defaultAttributes: attributes)
      privacyPolicyTextView.delegate = self
    }
  }
  
  @IBOutlet private weak var termsOfUseTextView: UITextView! {
    didSet {
      let htmlString = L("sign_agree_tof_gdpr")
      let attributes: [NSAttributedStringKey : Any] = [NSAttributedStringKey.font: UIFont.regular16(),
                                                       NSAttributedStringKey.foregroundColor: UIColor.blackPrimaryText()]
      termsOfUseTextView.attributedText = NSAttributedString.string(withHtml: htmlString,
                                                                    defaultAttributes: attributes)
      termsOfUseTextView.delegate = self
    }
  }
  
  @IBOutlet private weak var latestNewsTextView: UITextView! {
    didSet {
      let text = L("sign_agree_news_gdpr")
      let attributes: [NSAttributedStringKey : Any] = [NSAttributedStringKey.font: UIFont.regular16(),
                                                       NSAttributedStringKey.foregroundColor: UIColor.blackPrimaryText()]
      latestNewsTextView.attributedText = NSAttributedString(string: text, attributes: attributes)
    }
  }
  
  @IBOutlet private weak var topToContentConstraint: NSLayoutConstraint!

  typealias SuccessHandler = (MWMSocialTokenType) -> Void
  typealias ErrorHandler = (MWMAuthorizationError) -> Void
  typealias CompletionHandler = (AuthorizationViewController) -> Void

  private let sourceComponent: MWMAuthorizationSource
  private let successHandler: SuccessHandler?
  private let errorHandler: ErrorHandler?
  private let completionHandler: CompletionHandler?

  @objc
  init(barButtonItem: UIBarButtonItem?, sourceComponent: MWMAuthorizationSource, successHandler: SuccessHandler? = nil, errorHandler: ErrorHandler? = nil, completionHandler: CompletionHandler? = nil) {
    self.sourceComponent = sourceComponent
    self.successHandler = successHandler
    self.errorHandler = errorHandler
    self.completionHandler = completionHandler
    transitioningManager = AuthorizationTransitioningManager(barButtonItem: barButtonItem)
    super.init(nibName: toString(type(of: self)), bundle: nil)
    transitioningDelegate = transitioningManager
    modalPresentationStyle = .custom
  }

  @objc
  init(popoverSourceView: UIView? = nil, sourceComponent: MWMAuthorizationSource, permittedArrowDirections: UIPopoverArrowDirection = .unknown, successHandler: SuccessHandler? = nil, errorHandler: ErrorHandler? = nil, completionHandler: CompletionHandler? = nil) {
    self.sourceComponent = sourceComponent
    self.successHandler = successHandler
    self.errorHandler = errorHandler
    self.completionHandler = completionHandler
    transitioningManager = AuthorizationTransitioningManager(popoverSourceView: popoverSourceView, permittedArrowDirections: permittedArrowDirections)
    super.init(nibName: toString(type(of: self)), bundle: nil)
    transitioningDelegate = transitioningManager
    modalPresentationStyle = .custom
  }

  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    iPadSpecific {
      topToContentConstraint.isActive = false
    }
  }

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    iPadSpecific {
      preferredContentSize = contentView.systemLayoutSizeFitting(preferredContentSize, withHorizontalFittingPriority: .fittingSizeLevel, verticalFittingPriority: .fittingSizeLevel)
    }
  }

  @IBAction func onCancel() {
    Statistics.logEvent(kStatUGCReviewAuthDeclined)
    errorHandler?(.cancelled)
    onClose()
  }

  private func onClose() {
    dismiss(animated: true)
    completionHandler?(self)
  }
  
  private func getProviderStatStr(type: MWMSocialTokenType) -> String {
    switch type {
    case .facebook: return kStatFacebook
    case .google: return kStatGoogle
    case .phone: return kStatPhone
    }
  }

  private func process(error: Error, type: MWMSocialTokenType) {
    Statistics.logEvent(kStatUGCReviewAuthError, withParameters: [
      kStatProvider: getProviderStatStr(type: type),
      kStatError: error.localizedDescription,
    ])
    textLabel.text = L("profile_authorization_error")
    Crashlytics.sharedInstance().recordError(error)
  }

  private func process(token: String, type: MWMSocialTokenType) {
    Statistics.logEvent(kStatUGCReviewAuthExternalRequestSuccess, withParameters: [kStatProvider: getProviderStatStr(type: type)])
    ViewModel.authenticate(withToken: token,
                           type: type,
                           privacyAccepted: privacyPolicyCheck.isChecked,
                           termsAccepted: termsOfUseCheck.isChecked,
                           promoAccepted: latestNewsCheck.isChecked,
                           source: sourceComponent) { success in
      if success {
        self.successHandler?(type)
      } else {
        self.errorHandler?(.passportError)
      }
    }
    onClose()
  }
}

extension AuthorizationViewController: GIDSignInUIDelegate {
}

extension AuthorizationViewController: GIDSignInDelegate {
  func sign(_: GIDSignIn!, didSignInFor user: GIDGoogleUser!, withError error: Error!) {
    if let error = error {
      process(error: error, type: .google)
    } else {
      process(token: user.authentication.idToken, type: .google)
    }
  }
}

extension AuthorizationViewController: UITextViewDelegate {
  func textView(_ textView: UITextView, shouldInteractWith URL: URL, in characterRange: NSRange) -> Bool {
    let safari = SFSafariViewController(url: URL)
    self.present(safari, animated: true, completion: nil)
    return false;
  }
}

extension AuthorizationViewController: UIGestureRecognizerDelegate {
  func gestureRecognizer(_ gestureRecognizer: UIGestureRecognizer, shouldReceive touch: UITouch) -> Bool {
    return !contentView.point(inside: touch.location(in: contentView), with: nil)
  }
}
