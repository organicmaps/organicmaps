class SubscriptionViewBuilder {
  enum SuccessDialog {
    case goToCatalog
    case success
    case none
  }

  static func build(type: SubscriptionGroupType,
                    parentViewController: UIViewController,
                    source: String,
                    successDialog: SuccessDialog,
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
      parentViewController.dismiss(animated: true) {
        completion?(true);
      }
      switch successDialog {
      case .goToCatalog:
        let successDialog = SubscriptionGoToCatalogViewController(type, onOk: {
          parentViewController.dismiss(animated: true)
          let webViewController = CatalogWebViewController.catalogFromAbsoluteUrl(nil, utm: .none)
          parentViewController.navigationController?.pushViewController(webViewController, animated: true)
        }) {
          parentViewController.dismiss(animated: true)
        }
        parentViewController.present(successDialog, animated: true)
      case .success:
        let successDialog = SubscriptionSuccessViewController(type) {
          parentViewController.dismiss(animated: true)
        }
        parentViewController.present(successDialog, animated: true)
      case .none:
        break;
      }
    }
    subscribeViewController.onCancel = {
      parentViewController.dismiss(animated: true) {
        completion?(false)
      }
    }
    return subscribeViewController
  }
}
