@objc class PromoDiscoveryCampaign: NSObject, IPromoCampaign {
  enum Group: Int {
    case discoverCatalog = 0
    case downloadSamples
    case buySubscription
  }

  private var adapter: PromoDiscoveryCampaignAdapter

  let group: Group
  let url: URL?

  var enabled: Bool {
    return adapter.canShowTipButton() && Alohalytics.isFirstSession();
  }

  required override init() {
    adapter = PromoDiscoveryCampaignAdapter()
    group = Group(rawValue: adapter.type) ?? .discoverCatalog
    url = adapter.url
  }
}
