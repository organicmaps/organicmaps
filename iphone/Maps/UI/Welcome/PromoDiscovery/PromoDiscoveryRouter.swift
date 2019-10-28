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
    let subscribeViewController = AllPassSubscriptionViewController()
    subscribeViewController.onSubscribe = { [weak self] in
      self?.rootViewController?.dismiss(animated: true)
      let successDialog = SubscriptionSuccessViewController(.allPass) { [weak self] in
        self?.rootViewController?.dismiss(animated: true)
      }
      self?.rootViewController?.present(successDialog, animated: true)
    }
    subscribeViewController.onCancel = { [weak self] in
      self?.rootViewController?.dismiss(animated: true)
    }

    viewController?.present(subscribeViewController, animated: true)
  }

  private func presentPromoDiscoveryFree() {
    let webViewController = CatalogWebViewController.catalogFromAbsoluteUrl(campaign.url, utm: .bookmarksPageCatalogButton)
    rootViewController?.navigationController?.pushViewController(webViewController, animated: true)
    dissmiss()
  }
}
