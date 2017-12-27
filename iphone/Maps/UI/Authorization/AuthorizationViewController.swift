import FBSDKCoreKit
import FBSDKLoginKit
import GoogleSignIn

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

  @IBOutlet private weak var tapView: UIView! {
    didSet {
      iPadSpecific {
        tapView.removeFromSuperview()
      }
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
      textLabel.text = L("profile_authorization_message")
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
      googleButton.titleLabel?.font = UIFont.bold14()
      let gid = GIDSignIn.sharedInstance()!
      gid.delegate = self
      gid.uiDelegate = self
    }
  }

  @IBAction func googleSignIn() {
    GIDSignIn.sharedInstance().signIn()
  }

  private lazy var facebookButton: FBSDKLoginButton = {
    let button = FBSDKLoginButton()
    button.delegate = self
    button.loginBehavior = .systemAccount
    button.setAttributedTitle(NSAttributedString(string: "Facebook"), for: .normal)
    button.readPermissions = ["public_profile", "email"]
    return button
  }()

  @IBOutlet private weak var facebookButtonHolder: UIView! {
    didSet {
      facebookButton.translatesAutoresizingMaskIntoConstraints = false
      facebookButtonHolder.addSubview(facebookButton)
      facebookButton.removeConstraints(facebookButton.constraints)
      addConstraints(v1: facebookButton, v2: facebookButtonHolder)
      facebookButtonHolder.layer.cornerRadius = 8
      facebookButtonHolder.clipsToBounds = true
    }
  }

  private let completion: (() -> Void)?

  private func addConstraints(v1: UIView, v2: UIView) {
    [NSLayoutAttribute.top, .bottom, .left, .right].forEach {
      NSLayoutConstraint(item: v1, attribute: $0, relatedBy: .equal, toItem: v2, attribute: $0, multiplier: 1, constant: 0).isActive = true
    }
  }

  @objc init(barButtonItem: UIBarButtonItem?, completion: (() -> Void)? = nil) {
    self.completion = completion
    transitioningManager = AuthorizationTransitioningManager(barButtonItem: barButtonItem)
    super.init(nibName: toString(type(of: self)), bundle: nil)
    transitioningDelegate = transitioningManager
    modalPresentationStyle = .custom
  }

  @objc init(popoverSourceView: UIView? = nil, permittedArrowDirections: UIPopoverArrowDirection = .unknown, completion: (() -> Void)? = nil) {
    self.completion = completion
    transitioningManager = AuthorizationTransitioningManager(popoverSourceView: popoverSourceView, permittedArrowDirections: permittedArrowDirections)
    super.init(nibName: toString(type(of: self)), bundle: nil)
    transitioningDelegate = transitioningManager
    modalPresentationStyle = .custom
  }

  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)

    let fbImage = facebookButton.subviews.first(where: { $0 is UIImageView && $0.frame != facebookButton.frame })
    fbImage?.frame = CGRect(x: 16, y: 8, width: 24, height: 24)
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    preferredContentSize = contentView.size
  }

  @IBAction func onCancel() {
    Statistics.logEvent(kStatUGCReviewAuthDeclined)
    dismiss(animated: true, completion: {
      (UIApplication.shared.keyWindow?.rootViewController as?
        UINavigationController)?.popToRootViewController(animated: true)
    })
  }

  private func process(error: Error, type: MWMSocialTokenType) {
    Statistics.logEvent(kStatUGCReviewAuthError, withParameters: [
      kStatProvider: type == .facebook ? kStatFacebook : kStatGoogle,
      kStatError: error.localizedDescription,
    ])
    textLabel.text = L("profile_authorization_error")
    Crashlytics.sharedInstance().recordError(error)
  }

  private func process(token: String, type: MWMSocialTokenType) {
    Statistics.logEvent(kStatUGCReviewAuthExternalRequestSuccess, withParameters: [kStatProvider: type == .facebook ? kStatFacebook : kStatGoogle])
    ViewModel.authenticate(withToken: token, type: type)
    dismiss(animated: true, completion: completion)
  }
}

extension AuthorizationViewController: FBSDKLoginButtonDelegate {
  func loginButton(_: FBSDKLoginButton!, didCompleteWith result: FBSDKLoginManagerLoginResult!, error: Error!) {
    if let error = error {
      process(error: error, type: .facebook)
    } else if let token = result?.token {
      process(token: token.tokenString, type: .facebook)
    }
  }

  func loginButtonDidLogOut(_: FBSDKLoginButton!) {}
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
