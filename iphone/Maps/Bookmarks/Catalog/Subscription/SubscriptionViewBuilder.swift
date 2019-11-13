class SubscriptionViewBuilder {
  static func build(type: SubscriptionGroupType,
                    parentViewController: UIViewController,
                    source: String,
                    completion: ((Bool) -> Void)?) -> UIViewController {
    let subscribeViewController: BaseSubscriptionViewController
    switch type {
    case .allPass:
      subscribeViewController = AllPassSubscriptionViewController()
    case .sightseeing:
      subscribeViewController = BookmarksSubscriptionViewController()
    }
    subscribeViewController.source = source
    subscribeViewController.onSubscribe = {
      completion?(true);
      parentViewController.dismiss(animated: true)
      let successDialog = SubscriptionSuccessViewController(type) {
        parentViewController.dismiss(animated: true)
      }
      parentViewController.present(successDialog, animated: true)
    }
    subscribeViewController.onCancel = {
      completion?(false)
      parentViewController.dismiss(animated: true)
    }
    return subscribeViewController
  }
}
