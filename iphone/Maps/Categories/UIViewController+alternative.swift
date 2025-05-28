extension UIViewController {
  func alternativeSizeClass<T>(iPhone: @autoclosure () -> T, iPad: @autoclosure () -> T) -> T {
    isIPad ? iPad() : iPhone()
  }

  func alternativeSizeClass(iPhone: () -> Void, iPad: () -> Void) {
    isIPad ? iPad() : iPhone()
  }
}
