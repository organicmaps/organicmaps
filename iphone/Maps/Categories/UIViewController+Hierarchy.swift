extension UIViewController {
  static func topViewController() -> UIViewController {
    let window = UIApplication.shared.delegate!.window!!
    return (window.rootViewController as! UINavigationController).topViewController!
  }
}
