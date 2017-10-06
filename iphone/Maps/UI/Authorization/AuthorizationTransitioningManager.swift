final class AuthorizationTransitioningManager: NSObject, UIViewControllerTransitioningDelegate {
  private let popoverSourceView: UIView?
  private let permittedArrowDirections: UIPopoverArrowDirection?
  private let barButtonItem: UIBarButtonItem?

  init(barButtonItem: UIBarButtonItem?) {
    self.barButtonItem = barButtonItem
    popoverSourceView = nil
    permittedArrowDirections = nil
    super.init()
  }

  init(popoverSourceView: UIView?, permittedArrowDirections: UIPopoverArrowDirection?) {
    barButtonItem = nil
    self.popoverSourceView = popoverSourceView
    self.permittedArrowDirections = permittedArrowDirections
    super.init()
  }

  func presentationController(forPresented presented: UIViewController, presenting: UIViewController?, source _: UIViewController) -> UIPresentationController? {
    return alternative(iPhone: { () -> UIPresentationController in
      AuthorizationiPhonePresentationController(presentedViewController: presented,
                                                presenting: presenting)
    },
    iPad: { () -> UIPresentationController in
      let popover = AuthorizationiPadPresentationController(presentedViewController: presented,
                                                            presenting: presenting)
      if let barButtonItem = self.barButtonItem {
        popover.barButtonItem = barButtonItem
      } else {
        popover.sourceView = self.popoverSourceView!
        popover.sourceRect = self.popoverSourceView!.bounds
        popover.permittedArrowDirections = self.permittedArrowDirections!
      }
      return popover
    })()
  }

  func animationController(forPresented _: UIViewController, presenting _: UIViewController, source _: UIViewController) -> UIViewControllerAnimatedTransitioning? {
    return AuthorizationTransitioning(isPresentation: true)
  }

  func animationController(forDismissed _: UIViewController) -> UIViewControllerAnimatedTransitioning? {
    return AuthorizationTransitioning(isPresentation: false)
  }
}
