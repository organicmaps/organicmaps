protocol SubscriptionViewProtocol: AnyObject {
  var isLoadingHidden: Bool { get set }
  var presenter: SubscriptionPresenterProtocol! { get set }
  func setModel(_ model: SubscriptionViewModel)
}

enum SubscriptionViewModel {
  struct TrialData {
    let price: String
  }

  struct SubscriptionData {
    let price: String
    let title: String
    let period: SubscriptionPeriod
    let hasDiscount: Bool
    let discount: String
  }

  case loading
  case subsctiption([SubscriptionData])
  case trial(TrialData)
}
