final class InAppPurchase {
  static func paidRoutePurchase(serverId: String,
                                productId: String) -> IPaidRoutePurchase {
    let validation = MWMPurchaseValidation(vendorId: BOOKMARKS_VENDOR)
    let billing = InAppBilling()
    return PaidRoutePurchase(serverId: serverId,
                             productId: productId,
                             purchaseValidation: validation,
                             billing: billing)
  }
}
