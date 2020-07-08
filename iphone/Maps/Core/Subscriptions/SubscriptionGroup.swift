@objc enum SubscriptionGroupType: Int {
  case allPass
  case city

  init?(serverId: String) {
    switch serverId {
    case MWMPurchaseManager.bookmarksSubscriptionServerId():
      self = .city
    case MWMPurchaseManager.allPassSubscriptionServerId():
      self = .allPass
    default:
      return nil
    }
  }

  init(catalogURL: URL) {
    guard let urlComponents = URLComponents(url: catalogURL, resolvingAgainstBaseURL: false) else {
      self = .allPass
      return
    }
    let subscriptionGroups = urlComponents.queryItems?
      .filter { $0.name == "groups" }
      .map { $0.value ?? "" }

    if subscriptionGroups?.first(where: { $0 == MWMPurchaseManager.allPassSubscriptionServerId() }) != nil {
      self = .allPass
    } else if subscriptionGroups?.first(where: { $0 == MWMPurchaseManager.bookmarksSubscriptionServerId() }) != nil {
      self = .city
    } else {
      self = .allPass
    }
  }

  var serverId: String {
    switch self {
    case .city:
      return MWMPurchaseManager.bookmarksSubscriptionServerId()
    case .allPass:
      return MWMPurchaseManager.allPassSubscriptionServerId()
    }
  }
}

protocol ISubscriptionGroup {
  var count: Int { get }
  subscript(period: SubscriptionPeriod) -> ISubscriptionGroupItem? { get }
}

class SubscriptionGroup: ISubscriptionGroup {
  private var subscriptions: [ISubscriptionGroupItem]
  var count: Int {
    return subscriptions.count
  }

  init(subscriptions: [ISubscription]) {
    let formatter = NumberFormatter()
    formatter.locale = subscriptions.first?.priceLocale
    formatter.numberStyle = .currency

    let weekCycle = NSDecimalNumber(value: 7.0)
    let mounthCycle = NSDecimalNumber(value: 30.0)
    let yearCycle = NSDecimalNumber(value: 12.0 * 30.0)
    var rates: [NSDecimalNumber] = []
    var maxPriceRate: NSDecimalNumber = NSDecimalNumber.minimum

    maxPriceRate = subscriptions.reduce(into: maxPriceRate) { result, item in
      let price = item.price
      var rate: NSDecimalNumber = NSDecimalNumber()

      switch item.period {
      case .year:
        rate = price.dividing(by: yearCycle)
      case .month:
        rate = price.dividing(by: mounthCycle)
      case .week:
        rate = price.dividing(by: weekCycle)
      case .unknown:
        rate = price
      }
      result = rate.compare(result) == .orderedDescending ? rate : result
      rates.append(rate)
    }
    self.subscriptions = []
    for (idx, subscription) in subscriptions.enumerated() {
      let rate = rates[idx]
      let discount = NSDecimalNumber(value: 1).subtracting(rate.dividing(by: maxPriceRate)).multiplying(by: 100)
      self.subscriptions.append(SubscriptionGroupItem(subscription, discount: discount, formatter: formatter))
    }
  }

  subscript(period: SubscriptionPeriod) -> ISubscriptionGroupItem? {
    return subscriptions.first { (item) -> Bool in
      item.period == period
    }
  }
}
