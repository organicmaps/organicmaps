extension UIViewController {
  func checkInvalidSubscription() {
    MWMBookmarksManager.shared().check { [weak self] hasInvalidSubscription in
      guard hasInvalidSubscription else {
        return
      }

      let onSubscribe = {
        self?.dismiss(animated: true)
        let subscriptionDialog = BookmarksSubscriptionViewController()
        subscriptionDialog.onSubscribe = { [weak self] in
          self?.dismiss(animated: true)
        }
        subscriptionDialog.onCancel = { [weak self] in
          self?.dismiss(animated: true) {
            self?.checkInvalidSubscription()
          }
        }
        self?.present(subscriptionDialog, animated: true)
      }
      let onDelete = {
        self?.dismiss(animated: true)
        MWMBookmarksManager.shared().deleteInvalidCategories()
      }
      let subscriptionExpiredDialog = BookmarksSubscriptionExpiredViewController(onSubscribe: onSubscribe, onDelete: onDelete)
      self?.present(subscriptionExpiredDialog, animated: true)
    }
  }
}

