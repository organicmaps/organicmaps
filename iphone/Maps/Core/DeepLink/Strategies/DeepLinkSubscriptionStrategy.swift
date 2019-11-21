class DeepLinkSubscriptionStrategy: IDeepLinkHandlerStrategy {
  var deeplinkURL: DeepLinkURL
  private var data: DeepLinkSubscriptionData

  init(url: DeepLinkURL, data: DeepLinkSubscriptionData) {
    self.deeplinkURL = url
    self.data = data
  }

  func execute() {
    guard let mapViewController = MapViewController.shared() else {
      return;
    }
    let type: SubscriptionGroupType
    switch data.deliverable {
    case MWMPurchaseManager.bookmarksSubscriptionServerId():
      type = .sightseeing
    case MWMPurchaseManager.allPassSubscriptionServerId():
      type = .allPass
    default:
      LOG(.error, "Deliverable is wrong: \(deeplinkURL.url)");
      return;
    }
    mapViewController.dismiss(animated: false, completion: nil)
    mapViewController.navigationController?.popToRootViewController(animated: false)
    let subscriptionViewController = SubscriptionViewBuilder.build(type: type,
                                                         parentViewController: mapViewController,
                                                         source: kStatDeeplink,
                                                         openCatalog: true,
                                                         completion: nil)
    mapViewController.present(subscriptionViewController, animated: true, completion: nil)
    sendStatisticsOnSuccess(type: kStatSubscription)
  }
}
