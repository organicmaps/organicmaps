@objc class PromoDiscoveryCampaign: NSObject, IABTest {
  enum Group: Int {
    case discoverCatalog = 0
    case downloadSamples
    case buySubscription
  }

  private var adapter: PromoDiscoveryCampaignAdapter

  let group: Group
  let url: URL?
  @objc private(set) var hasBeenActivated: Bool = false

  var enabled: Bool {
    return adapter.canShowTipButton() && Alohalytics.isFirstSession() && !hasBeenActivated;
  }

  required override init() {
    adapter = PromoDiscoveryCampaignAdapter()
    group = Group(rawValue: adapter.type) ?? .discoverCatalog
    url = adapter.url
  }

  func onActivate() {
    hasBeenActivated = true
  }
}
