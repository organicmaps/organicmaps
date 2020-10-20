class SubscriptionViewBuilder {
  static func build(type: SubscriptionGroupType,
                    parentViewController: UIViewController,
                    source: String,
                    successDialog: SubscriptionSuccessDialog,
                    completion: ((Bool) -> Void)?) -> UIViewController {
    switch type {
    case .city:
      return CitySubscriptionBuilder.build(parentViewController: parentViewController,
                                           source: source,
                                           successDialog: successDialog,
                                           subscriptionGroupType: type,
                                           completion: completion)
    case .allPass:
      return AllPassSubscriptionBuilder.build(parentViewController: parentViewController,
                                              source: source,
                                              successDialog: successDialog,
                                              subscriptionGroupType: type,
                                              completion: completion)
    }
  }

  static func buildLonelyPlanet(parentViewController: UIViewController,
                                source: String,
                                successDialog: SubscriptionSuccessDialog,
                                completion: ((Bool) -> Void)?) -> UIViewController {
    return AllPassSubscriptionBuilder.build(parentViewController: parentViewController,
                                            source: source,
                                            successDialog: successDialog,
                                            subscriptionGroupType: .allPass,
                                            completion: completion,
                                            startPage: 4)
  }
}
