extension UIViewController {
  func checkInvalidSubscription(_ completion: ((_ deleted: Bool) -> Void)? = nil) {
    MWMBookmarksManager.shared().check { [weak self] hasInvalidSubscription in
      guard hasInvalidSubscription else {
        return
      }

      let onSubscribe = {
        self?.dismiss(animated: true)
        let subscriptionDialog = AllPassSubscriptionViewController()
        subscriptionDialog.onSubscribe = { [weak self] in
          self?.dismiss(animated: true)
        }
        subscriptionDialog.onCancel = { [weak self] in
          self?.dismiss(animated: true) {
            self?.checkInvalidSubscription(completion)
          }
        }
        self?.present(subscriptionDialog, animated: true)
        completion?(false)
      }
      let onDelete = {
        self?.dismiss(animated: true)
        MWMBookmarksManager.shared().deleteInvalidCategories()
        completion?(true)
      }
      let subscriptionExpiredDialog = SubscriptionExpiredViewController(onSubscribe: onSubscribe, onDelete: onDelete)
      self?.present(subscriptionExpiredDialog, animated: true)
    }
  }
}

