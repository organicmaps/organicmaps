@objc class PromoAfterBookingCampaign: NSObject, IPromoCampaign {
  private var adapter: PromoAfterBookingCampaignAdapter?

  @objc let promoId: String
  @objc let promoUrl: String
  @objc let pictureUrl: String

  var enabled: Bool {
    return adapter != nil
  }

  required override init() {
    adapter = PromoAfterBookingCampaignAdapter()
    guard let adapter = adapter else{
      promoId = ""
      promoUrl = ""
      pictureUrl = ""
      return;
    }

    promoId = adapter.promoId
    promoUrl = adapter.promoUrl
    pictureUrl = adapter.pictureUrl
  }
}
