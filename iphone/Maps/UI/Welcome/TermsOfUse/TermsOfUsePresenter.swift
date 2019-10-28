protocol ITermsOfUsePresenter: IWelcomePresenter {
  func configure()
  func onNext()
}

class TermsOfUsePresenter {
  private weak var view: ITermsOfUseView?
  private let router: WelcomeRouterProtocol

  let privacyPolicyLink = MWMAuthorizationViewModel.privacyPolicyLink()
  let termsOfUseLink = MWMAuthorizationViewModel.termsOfUseLink()

  init(view: ITermsOfUseView, router: WelcomeRouterProtocol) {
    self.view = view
    self.router = router
  }
}

extension TermsOfUsePresenter: ITermsOfUsePresenter {
  func configure() {
    view?.setTitleImage(UIImage(named: "img_onboarding_travelbuddy"))
    view?.setTitle(L("new_onboarding_step1_header"))
    view?.setText(L("new_onboarding_step1_message"))
    view?.setPrivacyPolicyTitle(String(coreFormat: L("sign_agree_pp_gdpr"), arguments: [privacyPolicyLink]))
    view?.setTermsOfUseTitle(String(coreFormat: L("sign_agree_tof_gdpr"), arguments: [termsOfUseLink]))
  }

  func key() -> String {
    return ""
  }

  func onAppear() {
    Statistics.logEvent("OnStart_MapsMeConsent_shown")
  }

  func onNext() {
    WelcomeStorage.shouldShowTerms = false
    WelcomeStorage.privacyPolicyLink = privacyPolicyLink
    WelcomeStorage.termsOfUseLink = termsOfUseLink
    WelcomeStorage.acceptTime = Date()
    router.onNext()
    Statistics.logEvent("OnStart_MapsMeConsent_accepted")
  }

  func onClose() {

  }
}
