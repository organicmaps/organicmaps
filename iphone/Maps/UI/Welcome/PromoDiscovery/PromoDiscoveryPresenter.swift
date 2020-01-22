protocol IPromoRouterPresenter: IWelcomePresenter {

}

class PromoDiscoveryPresenter {
  private weak var viewController: IWelcomeView?
  private let router: IPromoDiscoveryRouter
  private let campaign: PromoDiscoveryCampaign
  private var statType: String = ""

  init(viewController: IWelcomeView,
       router: IPromoDiscoveryRouter,
       campaign: PromoDiscoveryCampaign) {
    self.viewController = viewController
    self.router = router
    self.campaign = campaign;
  }
}

extension PromoDiscoveryPresenter: IPromoRouterPresenter {
  func configure() {
    switch campaign.group {
    case .discoverCatalog:
      viewController?.setTitleImage(UIImage(named: "img_onboarding_subscribeguides"))
      viewController?.setTitle(L("new_onboarding_step5.1_header"))
      viewController?.setText(L("new_onboarding_step5.1_message"))
      viewController?.setNextButtonTitle(L("new_onboarding_step5.1_button"))
      statType = kStatOnboardingCatalog
    case .buySubscription:
      viewController?.setTitleImage(UIImage(named: "img_onboarding_subscribeguides"))
      viewController?.setTitle(L("new_onboarding_step5.1_header"))
      viewController?.setText(L("new_onboarding_step5.2_message"))
      viewController?.setNextButtonTitle(L("new_onboarding_step5.2_button"))
      statType = kStatOnboardingSubscription
    case .downloadSamples:
      viewController?.setTitleImage(UIImage(named: "img_onboarding_samples"))
      viewController?.setTitle(L("new_onboarding_step5.1_header"))
      viewController?.setText(L("new_onboarding_step5.3_message"))
      viewController?.setNextButtonTitle(L("new_onboarding_step5.3_button"))
      statType = kStatOnboardingSample
    }
  }

  func onAppear() {
    Statistics.logEvent(kStatOnboardingScreenShow, withParameters: [kStatType: statType])
  }

  func onNext() {
    router.presentNext()
    Statistics.logEvent(kStatOnboardingScreenAccept, withParameters: [kStatType: statType])
  }

  func onClose() {
    router.dissmiss()
    Statistics.logEvent(kStatOnboardingScreenDecline, withParameters: [kStatType: statType])
  }
}
