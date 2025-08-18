extension UIViewController {
  func alternativeSizeClass<T>(iPhone: @autoclosure () -> T, iPad: @autoclosure () -> T) -> T {
    isiPad ? iPad() : iPhone()
  }

  func alternativeSizeClass(iPhone: () -> Void, iPad: () -> Void) {
    isiPad ? iPad() : iPhone()
  }
}
