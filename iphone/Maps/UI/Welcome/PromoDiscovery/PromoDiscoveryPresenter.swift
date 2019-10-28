protocol IPromoRouterPresenter: IWelcomePresenter {

}

class PromoDiscoveryPresenter {
  private weak var viewController: IWelcomeView?
  private let router: IPromoDiscoveryRouter
  private let campaign: PromoDiscoveryCampaign

  init(viewController: IWelcomeView,
       router: IPromoDiscoveryRouter,
       campaign: PromoDiscoveryCampaign) {
    self.viewController = viewController
    self.router = router
    self.campaign = campaign;
  }
}

extension PromoDiscoveryPresenter: IPromoRouterPresenter {
  func configure(){
    switch campaign.group {
    case .discoverCatalog:
      viewController?.setTitleImage(UIImage(named: "img_onboarding_subscribeguides"))
      viewController?.setTitle(L("new_onboarding_step5.1_header"))
      viewController?.setText(L("new_onboarding_step5.1_message"))
      viewController?.setNextButtonTitle(L("new_onboarding_step5.1_button"))
    case .buySubscription:
      viewController?.setTitleImage(UIImage(named: "img_onboarding_subscribeguides"))
      viewController?.setTitle(L("new_onboarding_step5.2_header"))
      viewController?.setText(L("new_onboarding_step5.2_message"))
      viewController?.setNextButtonTitle(L("new_onboarding_step5.2_button"))
    case .downloadSamples:
      viewController?.setTitleImage(UIImage(named: "img_onboarding_samples"))
      viewController?.setTitle(L("new_onboarding_step5.3_header"))
      viewController?.setText(L("new_onboarding_step5.3_message"))
      viewController?.setNextButtonTitle(L("new_onboarding_step5.3_button"))
    }
  }

  func key() -> String {
    return ""
  }

  func onAppear() {

  }

  func onNext() {
    router.presentNext()
  }

  func onClose() {
    router.dissmiss()
  }
}
