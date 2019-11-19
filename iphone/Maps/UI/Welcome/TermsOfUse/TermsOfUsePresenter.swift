protocol ITermsOfUsePresenter: IWelcomePresenter {
  func configure()
  func onNext()
}

class TermsOfUsePresenter {
  private weak var view: ITermsOfUseView?
  private let router: WelcomeRouterProtocol

  let privacyPolicyLink = User.privacyPolicyLink()
  let termsOfUseLink = User.termsOfUseLink()

  init(view: ITermsOfUseView, router: WelcomeRouterProtocol) {
    self.view = view
    self.router = router
  }
}

extension TermsOfUsePresenter: ITermsOfUsePresenter {
  func configure() {
    view?.setTitleImage(UIImage(named: "img_onboarding_travelbuddy"))
    view?.setTitle("\(L("new_onboarding_step1_header"))\n\(L("new_onboarding_step1_header_2"))")
    view?.setText(L("sign_message_gdpr"))
    view?.setPrivacyPolicyTitle(String(coreFormat: L("sign_agree_pp_gdpr"), arguments: [privacyPolicyLink]))
    view?.setTermsOfUseTitle(String(coreFormat: L("sign_agree_tof_gdpr"), arguments: [termsOfUseLink]))
  }

  func onAppear() {
    Statistics.logEvent(kStatOnboardingScreenShow, withParameters: [kStatType: kStatAgreement])
  }

  func onNext() {
    WelcomeStorage.shouldShowTerms = false
    WelcomeStorage.privacyPolicyLink = privacyPolicyLink
    WelcomeStorage.termsOfUseLink = termsOfUseLink
    WelcomeStorage.acceptTime = Date()
    router.onNext()
    Statistics.logEvent(kStatOnboardingScreenAccept, withParameters: [kStatType: kStatAgreement])
  }

  func onClose() {

  }
}
