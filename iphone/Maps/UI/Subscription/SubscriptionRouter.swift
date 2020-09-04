import SafariServices

protocol SubscriptionRouterProtocol: AnyObject {
  func showTerms()
  func showPrivacy()
  func subscribe()
  func cancel()
}

enum SubscriptionSuccessDialog {
  case goToCatalog
  case success
  case none
}

class SubscriptionRouter {
  private weak var viewController: UIViewController?
  private weak var parentViewController: UIViewController?
  private var successDialog: SubscriptionSuccessDialog
  private var completion: ((Bool) -> Void)?
  private var subscriptionGroupType: SubscriptionGroupType

  init(viewController: UIViewController,
       parentViewController: UIViewController,
       successDialog: SubscriptionSuccessDialog,
       subscriptionGroupType: SubscriptionGroupType,
       completion: ((Bool) -> Void)?) {
    self.viewController = viewController
    self.parentViewController = parentViewController
    self.successDialog = successDialog
    self.completion = completion
    self.subscriptionGroupType = subscriptionGroupType
  }
}

extension SubscriptionRouter: SubscriptionRouterProtocol {
  func showTerms() {
    guard let url = URL(string: User.termsOfUseLink()) else { return }
    let safari = SFSafariViewController(url: url)
    viewController?.present(safari, animated: true, completion: nil)
  }

  func showPrivacy() {
    guard let url = URL(string: User.privacyPolicyLink()) else { return }
    let safari = SFSafariViewController(url: url)
    viewController?.present(safari, animated: true, completion: nil)
  }

  func subscribe() {
    parentViewController?.dismiss(animated: true) { [weak self] in
      self?.completion?(true)
    }
    switch successDialog {
    case .goToCatalog:
      let successDialog = SubscriptionGoToCatalogViewController(subscriptionGroupType, onOk: { [parentViewController] in
        parentViewController?.dismiss(animated: true)
        let webViewController = CatalogWebViewController.catalogFromAbsoluteUrl(nil, utm: .none)
        parentViewController?.navigationController?.pushViewController(webViewController, animated: true)
      }) { [parentViewController] in
        parentViewController?.dismiss(animated: true)
      }
      parentViewController?.present(successDialog, animated: true)
    case .success:
      let successDialog = SubscriptionSuccessViewController(subscriptionGroupType) { [parentViewController] in
        parentViewController?.dismiss(animated: true)
      }
      parentViewController?.present(successDialog, animated: true)
    case .none:
      break
    }
  }

  func cancel() {
    parentViewController?.dismiss(animated: true) { [weak self] in
      self?.completion?(false)
    }
  }
}
