protocol IPromoDiscoveryRouter: class {
  func dissmiss()
  func presentNext()
}

class PromoDiscoveryRouter {
  private weak var viewController: UIViewController?
  private weak var rootViewController: MWMViewController?
  private let campaign: PromoDiscoveryCampaign

  init(viewController: UIViewController,
       rootViewController: MWMViewController?,
       campaign: PromoDiscoveryCampaign) {
    self.viewController = viewController
    self.rootViewController = rootViewController
    self.campaign = campaign
  }
}

extension PromoDiscoveryRouter: IPromoDiscoveryRouter {
  func dissmiss() {
    viewController?.dismiss(animated: true, completion: nil)
  }

  func presentNext() {
    switch campaign.group {
    case .discoverCatalog:
      presentPromoDiscoveryGuide()
    case .buySubscription:
      presentPromoDiscoverySubscribe()
    case .downloadSamples:
      presentPromoDiscoveryFree()
    }
  }

  private func presentPromoDiscoveryGuide() {
    let webViewController = CatalogWebViewController.catalogFromAbsoluteUrl(campaign.url, utm: .bookmarksPageCatalogButton)
    rootViewController?.navigationController?.pushViewController(webViewController, animated: true)
    dissmiss()
  }

  private func presentPromoDiscoverySubscribe() {
    guard let rootViewController = rootViewController else {
      return;
    }
    dissmiss()
    let subscribeViewController = SubscriptionViewBuilder.build(type: .allPass,
                                                                parentViewController: rootViewController,
                                                                source: kStatOnboardingGuidesSubscription,
                                                                successDialog: .goToCatalog,
                                                                completion: nil)
    rootViewController.present(subscribeViewController, animated: true)
  }

  private func presentPromoDiscoveryFree() {
    let webViewController = CatalogWebViewController.catalogFromAbsoluteUrl(campaign.url, utm: .bookmarksPageCatalogButton)
    rootViewController?.navigationController?.pushViewController(webViewController, animated: true)
    dissmiss()
  }
}
