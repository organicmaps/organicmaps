@objc class PromoAfterBookingCampaign: NSObject, IABTest {
  @objc var afterBookingData: PromoAfterBookingData {
    return PromoAfterBookingCampaignAdapter.afterBookingData()
  }

  @objc var enabled: Bool {
    return true;
  }

  required override init() {
  }
}
