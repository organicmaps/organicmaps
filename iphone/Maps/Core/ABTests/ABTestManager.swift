@objc protocol IABTestManager {
  @objc var promoDiscoveryCampaign: PromoDiscoveryCampaign { get }
  @objc var promoAfterBookingCampaign: PromoAfterBookingCampaign { get }
  @objc var paidRoutesSubscriptionCampaign: PaidRoutesSubscriptionCampaign { get }
  @objc var abTestBookingBackButtonColor: ABTestBookingBackButtonColor { get }
}

@objc class ABTestManager: NSObject, IABTestManager {
  private static var shared: IABTestManager = ABTestManager()

  @objc static func manager() -> IABTestManager {
    return shared
  }

  @objc lazy var promoDiscoveryCampaign: PromoDiscoveryCampaign = {
    return PromoDiscoveryCampaign()
  }()

  @objc lazy var promoAfterBookingCampaign: PromoAfterBookingCampaign = {
    return PromoAfterBookingCampaign()
  }()

  @objc lazy var paidRoutesSubscriptionCampaign: PaidRoutesSubscriptionCampaign = {
    return PaidRoutesSubscriptionCampaign()
  }()

  @objc lazy var abTestBookingBackButtonColor: ABTestBookingBackButtonColor = {
    return ABTestBookingBackButtonColor()
  }()
}
