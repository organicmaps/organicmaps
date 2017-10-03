final class AuthorizationTransitioningManager: NSObject, UIViewControllerTransitioningDelegate {
  private var popoverSourceView: UIView!
  private var permittedArrowDirections: UIPopoverArrowDirection!

  init(popoverSourceView: UIView?, permittedArrowDirections: UIPopoverArrowDirection?) {
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
      popover.sourceView = self.popoverSourceView
      popover.sourceRect = self.popoverSourceView.bounds
      popover.permittedArrowDirections = self.permittedArrowDirections
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
