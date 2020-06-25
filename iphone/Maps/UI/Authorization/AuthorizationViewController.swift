import FBSDKCoreKit
import FBSDKLoginKit
import GoogleSignIn
import SafariServices
import AuthenticationServices
import Firebase

@objc enum AuthorizationError: Int {
  case cancelled
  case passportError
}

@objc enum AuthorizationSource: Int {
  case afterSaveReview
  case bookmarksBackup
  case guideCatalogue
  case exportBookmarks
  case subscription

  var stat: String {
    switch self {
    case .afterSaveReview:
      return kStatAuthFromAfterSaveReview
    case .bookmarksBackup:
      return kStatAuthFromBookmarksBackup
    case .guideCatalogue:
      return kStatAuthFromGuideCatalogue
    case .exportBookmarks:
      return kStatAuthFromExportBookmarks
    case .subscription:
      return kStatAuthFromSubscription
    }
  }
}

@objc(MWMAuthorizationViewController)
final class AuthorizationViewController: MWMViewController {

  private let transitioningManager: AuthorizationTransitioningManager

  lazy var chromeView: UIView = {
    let view = UIView()
    view.styleName = "BlackStatusBarBackground"
    return view
  }()

  weak var containerView: UIView! {
    didSet {
      containerView.insertSubview(chromeView, at: 0)
    }
  }

  @IBOutlet private var contentView: UIView!
  @IBOutlet private var titleLabel: UILabel!
  @IBOutlet var separator: UIView!
  @IBOutlet private var textLabel: UILabel!

  @IBOutlet private var signInAppleContainerView: UIView!
  private var signInAppleButton: UIControl?

  @IBOutlet private var googleButton: UIButton! {
    didSet {
      googleButton.setTitle("Google", for: .normal)
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
    logStatStart(type: .google)
  }

  @IBOutlet private var facebookButton: UIButton! {
    didSet {
      facebookButton.isEnabled = false
    }
  }
  
  @IBAction func facebookSignIn() {
    let fbLoginManager = LoginManager()
    fbLoginManager.logIn(permissions: ["public_profile", "email"], from: self) { [weak self] (result, error) in
      if let error = error {
        self?.process(error: error, type: .facebook)
      } else if let token = result?.token {
        self?.process(token: token.tokenString, type: .facebook)
      }
    }
    logStatStart(type: .facebook)
  }

  @IBAction private func phoneSignIn() {
    let authVC = PhoneNumberAuthorizationViewController(success: { [unowned self] token in
      self.dismiss(animated: true)
      self.process(token: token, type: .phone)
    }, failure: { [unowned self] in
      self.dismiss(animated: true)
      self.process(error: NSError(domain: kMapsmeErrorDomain, code: 0), type: .phone)
      self.errorHandler?(.cancelled)
    })
    let navVC = MWMNavigationController(rootViewController: authVC)
    self.present(navVC, animated: true)
    logStatStart(type: .phone)
  }
  
  @IBOutlet private var phoneSignInButton: UIButton! {
    didSet {
      phoneSignInButton.isEnabled = false
    }
  }

  @IBOutlet private var privacyPolicyCheck: Checkmark!
  @IBOutlet private var termsOfUseCheck: Checkmark!
  @IBOutlet private var latestNewsCheck: Checkmark!
  
  @IBAction func onCheck(_ sender: Checkmark) {
    let allButtonsChecked = privacyPolicyCheck.isChecked &&
      termsOfUseCheck.isChecked
    
    googleButton.isEnabled = allButtonsChecked
    facebookButton.isEnabled = allButtonsChecked
    phoneSignInButton.isEnabled = allButtonsChecked
    signInAppleButton?.isEnabled = allButtonsChecked
    signInAppleButton?.alpha = allButtonsChecked ? 1 : 0.5
  }
  
  @IBOutlet private var privacyPolicyTextView: UITextView! {
    didSet {
      let htmlString = String(coreFormat: L("sign_agree_pp_gdpr"), arguments: [User.privacyPolicyLink()])
      privacyPolicyTextView.attributedText = NSAttributedString.string(withHtml: htmlString,
                                                                       defaultAttributes: [:])
      privacyPolicyTextView.delegate = self
    }
  }
  
  @IBOutlet private var termsOfUseTextView: UITextView! {
    didSet {
      let htmlString = String(coreFormat: L("sign_agree_tof_gdpr"), arguments: [User.termsOfUseLink()])
      termsOfUseTextView.attributedText = NSAttributedString.string(withHtml: htmlString,
                                                                    defaultAttributes: [:])
      termsOfUseTextView.delegate = self
    }
  }
  
  @IBOutlet private var latestNewsTextView: UITextView! {
    didSet {
      let text = L("sign_agree_news_gdpr")
      latestNewsTextView.attributedText = NSAttributedString(string: text, attributes: [:])
    }
  }
  
  @IBOutlet private var topToContentConstraint: NSLayoutConstraint!

  typealias SuccessHandler = (SocialTokenType) -> Void
  typealias ErrorHandler = (AuthorizationError) -> Void
  typealias CompletionHandler = (AuthorizationViewController) -> Void

  private let source: AuthorizationSource
  private let successHandler: SuccessHandler?
  private let errorHandler: ErrorHandler?
  private let completionHandler: CompletionHandler?

  @objc
  init(barButtonItem: UIBarButtonItem?,
       source: AuthorizationSource,
       successHandler: SuccessHandler? = nil,
       errorHandler: ErrorHandler? = nil,
       completionHandler: CompletionHandler? = nil) {
    self.source = source
    self.successHandler = successHandler
    self.errorHandler = errorHandler
    self.completionHandler = completionHandler
    transitioningManager = AuthorizationTransitioningManager(barButtonItem: barButtonItem)
    super.init(nibName: toString(type(of: self)), bundle: nil)
    transitioningDelegate = transitioningManager
    modalPresentationStyle = .custom
  }

  @objc
  init(popoverSourceView: UIView? = nil,
       source: AuthorizationSource,
       permittedArrowDirections: UIPopoverArrowDirection = .unknown,
       successHandler: SuccessHandler? = nil,
       errorHandler: ErrorHandler? = nil,
       completionHandler: CompletionHandler? = nil) {
    self.source = source
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
    if #available(iOS 13, *) {
      signInAppleContainerView.isHidden = false
      let button = ASAuthorizationAppleIDButton(type: .default, style: UIColor.isNightMode() ? .white : .black)
      button.isEnabled = false
      button.alpha = 0.5
      button.cornerRadius = 8
      button.addTarget(self, action: #selector(onAppleSignIn), for: .touchUpInside)
      signInAppleContainerView.addSubview(button)
      button.alignToSuperview()
      signInAppleButton = button
    }
  }

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)
    Statistics.logEvent(kStatAuthShown, withParameters: [kStatFrom: source.stat])
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    iPadSpecific {
      preferredContentSize = contentView.systemLayoutSizeFitting(preferredContentSize,
                                                                 withHorizontalFittingPriority: .fittingSizeLevel,
                                                                 verticalFittingPriority: .fittingSizeLevel)
    }
  }

  @available(iOS 13.0, *)
  @objc func onAppleSignIn() {
    let appleIDProvider = ASAuthorizationAppleIDProvider()
    let request = appleIDProvider.createRequest()
    request.requestedScopes = [.fullName, .email]

    let authorizationController = ASAuthorizationController(authorizationRequests: [request])
    authorizationController.delegate = self
    authorizationController.presentationContextProvider = self
    authorizationController.performRequests()
    logStatStart(type: .apple)
  }

  @IBAction func onCancel() {
    Statistics.logEvent(kStatAuthDeclined, withParameters: [kStatFrom: source.stat])
    errorHandler?(.cancelled)
    onClose()
  }

  private func onClose() {
    dismiss(animated: true)
    completionHandler?(self)
  }
  
  private func getProviderStatStr(type: SocialTokenType) -> String {
    switch type {
    case .facebook: return kStatFacebook
    case .google: return kStatGoogle
    case .phone: return kStatPhone
    case .apple: return kStatApple
    @unknown default:
      fatalError()
    }
  }

  private func process(error: Error, type: SocialTokenType) {
    Statistics.logEvent(kStatAuthError, withParameters: [
      kStatProvider: getProviderStatStr(type: type),
      kStatError: error.localizedDescription
    ])
    textLabel.text = L("profile_authorization_error")
    Crashlytics.crashlytics().record(error: error)
  }

  private func process(token: String,
                       type: SocialTokenType,
                       firstName: String = "",
                       lastName: String = "") {
    Statistics.logEvent(kStatAuthExternalRequestSuccess,
                        withParameters: [kStatProvider: getProviderStatStr(type: type)])
    User.authenticate(withToken: token,
                      type: type,
                      privacyAccepted: privacyPolicyCheck.isChecked,
                      termsAccepted: termsOfUseCheck.isChecked,
                      promoAccepted: latestNewsCheck.isChecked,
                      firstName: firstName,
                      lastName: lastName) { success in
                        self.logStatEnd(type: type, success: success)
                        if success {
                          self.successHandler?(type)
                        } else {
                          self.errorHandler?(.passportError)
                        }
    }
    onClose()
  }

  private func logStatStart(type: SocialTokenType) {
    var agreements = [kStatAgreePrivacy, kStatAgreeTerms]
    if latestNewsCheck.isChecked {
      agreements.append(kStatAgreeNews)
    }
    Statistics.logEvent(kStatAuthStart, withParameters: [kStatFrom: source.stat,
                                                         kStatProvider: getProviderStatStr(type: type),
                                                         kStatAgree: agreements])
  }

  private func logStatEnd(type: SocialTokenType, success: Bool) {
    let provider = getProviderStatStr(type: type)
    if success {
      Statistics.logEvent(kStatAuthRequestSucces, withParameters: [kStatProvider: provider])
    } else {
      Statistics.logEvent(kStatAuthError, withParameters: [kStatProvider: provider, kStatError: ""])
    }
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
    return false
  }
}

@available(iOS 13.0, *)
extension AuthorizationViewController: ASAuthorizationControllerDelegate {
  func authorizationController(controller: ASAuthorizationController, didCompleteWithAuthorization authorization: ASAuthorization) {
    switch authorization.credential {
    case let appleIDCredential as ASAuthorizationAppleIDCredential:
      guard let token = appleIDCredential.identityToken,
        let tokenString = String(data: token, encoding: .utf8) else { return }
      let fullName = appleIDCredential.fullName
      let userId = appleIDCredential.user
      let appleId = User.getAppleId()
      let firstName = fullName?.givenName ?? appleId?.firstName ?? ""
      let lastName = fullName?.familyName ?? appleId?.lastName ?? ""
      User.setAppleId(AppleId(userId: userId, firstName: firstName, lastName: lastName))
      process(token: tokenString, type: .apple, firstName: firstName, lastName: lastName)
    default:
      break
    }
  }

  func authorizationController(controller: ASAuthorizationController, didCompleteWithError error: Error) {
    process(error: error, type: .apple)
  }
}

@available(iOS 13.0, *)
extension AuthorizationViewController: ASAuthorizationControllerPresentationContextProviding {
  func presentationAnchor(for controller: ASAuthorizationController) -> ASPresentationAnchor {
    return self.view.window!
  }
}
