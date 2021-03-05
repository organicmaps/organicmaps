class SubscriptionViewBuilder {
  static func build(type: SubscriptionGroupType,
                    parentViewController: UIViewController,
                    successDialog: SubscriptionSuccessDialog,
                    completion: ((Bool) -> Void)?) -> UIViewController {
    switch type {
    case .city:
      return CitySubscriptionBuilder.build(parentViewController: parentViewController,
                                           successDialog: successDialog,
                                           subscriptionGroupType: type,
                                           completion: completion)
    case .allPass:
      return AllPassSubscriptionBuilder.build(parentViewController: parentViewController,
                                              successDialog: successDialog,
                                              subscriptionGroupType: type,
                                              completion: completion)
    }
  }

  static func buildLonelyPlanet(parentViewController: UIViewController,
                                successDialog: SubscriptionSuccessDialog,
                                completion: ((Bool) -> Void)?) -> UIViewController {
    return AllPassSubscriptionBuilder.build(parentViewController: parentViewController,
                                            successDialog: successDialog,
                                            subscriptionGroupType: .allPass,
                                            completion: completion,
                                            startPage: 4)
  }
}
