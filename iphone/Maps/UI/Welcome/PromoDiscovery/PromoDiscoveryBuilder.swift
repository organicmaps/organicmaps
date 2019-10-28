class PromoDiscoveryBuilder {
  static func build(rootViewController:MWMViewController?,
                    campaign: PromoDiscoveryCampaign) -> UIViewController {
    let sb = UIStoryboard.instance(.welcome)
    let vc = sb.instantiateViewController(ofType: WelcomeViewController.self);

    let router = PromoDiscoveryRouter(viewController: vc,
                                      rootViewController: rootViewController,
                                      campaign: campaign)
    let presenter = PromoDiscoveryPresenter(viewController: vc,
                                            router: router,
                                            campaign: campaign)
    vc.presenter = presenter

    return vc
  }
}
