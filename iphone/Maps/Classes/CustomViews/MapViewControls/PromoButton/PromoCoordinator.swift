import UIKit

@objc class PromoCoordinator: NSObject {
  @objc enum PromoType: Int{
    case crown
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
  }

  private func presentPromoDiscoveryOnboarding(completion: @escaping () -> Void) {
    let vc = PromoDiscoveryBuilder.build(rootViewController: mapViewController, campaign: campaign)
    vc.modalPresentationStyle = .fullScreen
    mapViewController?.present(vc, animated: true, completion: {
      completion()
    })
  }
}
