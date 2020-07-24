extension UIViewController {
  func checkInvalidSubscription(_ completion: ((_ deleted: Bool) -> Void)? = nil) {
    BookmarksManager.shared().check { [weak self] hasInvalidSubscription in
      guard hasInvalidSubscription else {
        return
      }

      let onSubscribe = {
        self?.dismiss(animated: true)
        guard let parentViewController = self else {
          return
        }
        let subscriptionDialog = AllPassSubscriptionBuilder.build(parentViewController: parentViewController,
                                                                  source: kStatWebView,
                                                                  successDialog: .none,
                                                                  subscriptionGroupType: .allPass) { result in
                                                                    if !result {
                                                                      self?.checkInvalidSubscription(completion)
                                                                    }
        }
        self?.present(subscriptionDialog, animated: true)
        completion?(false)
      }
      let onDelete = {
        self?.dismiss(animated: true)
        BookmarksManager.shared().deleteExpiredCategories()
        completion?(true)
      }
      let subscriptionExpiredDialog = SubscriptionExpiredViewController(onSubscribe: onSubscribe, onDelete: onDelete)
      self?.present(subscriptionExpiredDialog, animated: true)
    }
  }
}
