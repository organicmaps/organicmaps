@objc class PaidRoutesSubscriptionCampaign: NSObject, IABTest {
  enum SubscribeActionType: Int {
    case instant = 0
    case window
  }

  let actionType: SubscribeActionType
  lazy var testGroupStatName: String = {
    return actionType == .instant ? kStatTestGroup95PurchaseFlow1 : kStatTestGroup95PurchaseFlow2
  }()

  var enabled: Bool {
    return true
  }

  required override init() {
    let storageKey = "\(PaidRoutesSubscriptionCampaign.Type.self)"
    if let stored = UserDefaults.standard.value(forKey: storageKey) as? Int,
      let action = SubscribeActionType(rawValue: stored) {
      actionType = action;
    } else {
      let stored = arc4random()%2
      actionType = SubscribeActionType(rawValue: Int(stored)) ?? .instant
      UserDefaults.standard.set(stored, forKey: storageKey)
    }
  }
}
