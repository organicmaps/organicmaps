
@objc class PromoCoordinator: NSObject {
  @objc enum PromoType: Int{
    case discoveryGuide
    case discoverySubscribe
    case discoveryFree
  }

  private weak var mapViewController: MapViewController?
  let campaign: PromoDiscoveryCampaign

  @objc init(viewController: MapViewController, campaign: PromoDiscoveryCampaign) {
    self.mapViewController = viewController
    self.campaign = campaign
  }

  func onPromoButtonPress(completion: @escaping () -> Void) {
    presentPromoDiscoveryOnboarding(completion: completion)
    Statistics.logEvent(kStatMapSponsoredButtonClick, withParameters: [kStatTarget: kStatGuidesSubscription])
  }

  private func presentPromoDiscoveryOnboarding(completion: @escaping () -> Void) {
    let vc = PromoDiscoveryBuilder.build(rootViewController: mapViewController, campaign: campaign)
    mapViewController?.present(vc, animated: true, completion: {
      completion()
    })
  }
}
