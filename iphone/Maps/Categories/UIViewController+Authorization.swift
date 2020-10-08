@objc enum AuthorizationResult: Int {
  case succes
  case error
  case cancel
}
extension UIViewController {
  @objc func signup(anchor: UIView, source: AuthorizationSource, onComplete: @escaping (AuthorizationResult) -> Void) {
    if User.isAuthenticated() {
      onComplete(.succes)
    } else {
      let authVC = AuthorizationViewController(popoverSourceView: anchor,
                                               source: source,
                                               permittedArrowDirections: .any,
                                               successHandler: { _ in onComplete(.succes) },
                                               errorHandler: { onComplete($0 == .passportError ? .error : .cancel) })
      present(authVC, animated: true, completion: nil)
    }
  }
}
