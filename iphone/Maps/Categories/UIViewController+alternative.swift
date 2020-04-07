extension UIViewController {
  func alternativeSizeClass<T>(iPhone: @autoclosure () -> T, iPad: @autoclosure () -> T) -> T {
    if traitCollection.verticalSizeClass == .regular && traitCollection.horizontalSizeClass == .regular {
      return iPad()
    }
    return iPhone()
  }

  func alternativeSizeClass(iPhone: () -> Void, iPad: () -> Void) {
    if traitCollection.verticalSizeClass == .regular && traitCollection.horizontalSizeClass == .regular {
      iPad()
    } else {
      iPhone()
    }
  }
}
