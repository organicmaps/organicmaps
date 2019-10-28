@objc protocol IPromoCampaignManager {
  @objc var promoDiscoveryCampaign: PromoDiscoveryCampaign { get }
  @objc var promoAfterBookingCampaign: PromoAfterBookingCampaign { get }
}

@objc class PromoCampaignManager: NSObject, IPromoCampaignManager {
  private static var shared: IPromoCampaignManager = PromoCampaignManager()

  @objc static func manager() -> IPromoCampaignManager {
    return shared
  }

  @objc lazy var promoDiscoveryCampaign: PromoDiscoveryCampaign = {
    return PromoDiscoveryCampaign()
  }()

  @objc lazy var promoAfterBookingCampaign: PromoAfterBookingCampaign = {
    return PromoAfterBookingCampaign()
  }()
}
