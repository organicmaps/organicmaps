extension UIViewController {
  @objc static func topViewController() -> UIViewController {
    let window = UIApplication.shared.delegate!.window!!
    if var topController = window.rootViewController {
      while let presentedViewController = topController.presentedViewController {
        topController = presentedViewController
      }
      return topController
    }
    return (window.rootViewController as! UINavigationController).topViewController!
  }
}
