extension UIViewController {
  @objc func signup(anchor: UIView, onComplete: @escaping (Bool) -> Void) {
    if MWMAuthorizationViewModel.isAuthenticated() {
      onComplete(true)
    } else {
      let authVC = AuthorizationViewController(popoverSourceView: anchor,
                                               sourceComponent: .bookmarks,
                                               permittedArrowDirections: .any,
                                               successHandler: { _ in onComplete(true) },
                                               errorHandler: { _ in onComplete(false) })
      present(authVC, animated: true, completion: nil)
    }
  }
}
