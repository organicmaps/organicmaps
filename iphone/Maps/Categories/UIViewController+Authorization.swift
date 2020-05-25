extension UIViewController {
  @objc func signup(anchor: UIView, source: AuthorizationSource, onComplete: @escaping (Bool) -> Void) {
    if User.isAuthenticated() {
      onComplete(true)
    } else {
      let authVC = AuthorizationViewController(popoverSourceView: anchor,
                                               source: source,
                                               permittedArrowDirections: .any,
                                               successHandler: { _ in onComplete(true) },
                                               errorHandler: { _ in onComplete(false) })
      present(authVC, animated: true, completion: nil)
    }
  }
}
