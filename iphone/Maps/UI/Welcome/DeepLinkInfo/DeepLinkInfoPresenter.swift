protocol IDeepLinkInfoPresenter: IWelcomePresenter {

}

class DeepLinkInfoPresenter {
  private weak var view: IWelcomeView?
  private let router: DeepLinkInfoRouterProtocol
  private var deeplinkURL: URL?

  init(view: IWelcomeView, router: DeepLinkInfoRouterProtocol, deeplink: URL?) {
    self.view = view
    self.router = router
    self.deeplinkURL = deeplink
  }
}

extension DeepLinkInfoPresenter: IDeepLinkInfoPresenter {
  func configure() {
    guard let dlUrl = deeplinkURL, let host = dlUrl.host else { return }
    switch host {
    case "guides_page":
      view?.setTitle(L("onboarding_guide_direct_download_title"))
      view?.setText(L("onboarding_guide_direct_download_subtitle"))
      view?.setNextButtonTitle(L("onboarding_guide_direct_download_button"))
      view?.setTitleImage(UIImage(named: "img_onboarding_downloadmaps"))
      view?.isCloseButtonHidden = true
    case "catalogue":
      view?.setTitle(L("onboarding_bydeeplink_guide_title"))
      view?.setText(L("onboarding_bydeeplink_guide_subtitle"))
      view?.setNextButtonTitle(L("current_location_unknown_continue_button"))
      view?.setTitleImage(UIImage(named: "img_onboarding_downloadmaps"))
      view?.isCloseButtonHidden = true
    default:
      break
    }
  }

  func onAppear() {
    guard let dlUrl = deeplinkURL, let host = dlUrl.host else { return }
    Statistics.logEvent(kStatOnboardingDlShow, withParameters: [kStatType : host])
  }

  func onNext() {
    router.onNext()
    guard let dlUrl = deeplinkURL, let host = dlUrl.host else { return }
    Statistics.logEvent(kStatOnboardingDlAccept, withParameters: [kStatType : host])
  }

  func onClose() {

  }
}
