class AllPassSubscriptionBuilder {
  static func build(parentViewController: UIViewController,
                    source: String,
                    successDialog: SubscriptionSuccessDialog,
                    subscriptionGroupType: SubscriptionGroupType,
                    completion: ((Bool) -> Void)?) -> UIViewController {
    let viewController = AllPassSubscriptionViewController(nibName: nil, bundle: nil)
    let router = SubscriptionRouter(viewController: viewController,
                                    parentViewController: parentViewController,
                                    successDialog: successDialog,
                                    subscriptionGroupType: subscriptionGroupType,
                                    completion: completion)
    let interactor = SubscriptionInteractor(viewController: viewController,
                                            subscriptionManager: InAppPurchase.allPassSubscriptionManager,
                                            bookmarksManager: BookmarksManager.shared())
    let presenter = SubscriptionPresenter(view: viewController,
                                          router: router,
                                          interactor: interactor,
                                          subscriptionManager: InAppPurchase.allPassSubscriptionManager,
                                          source: source)

    interactor.presenter = presenter
    viewController.presenter = presenter

    return viewController
  }
}
