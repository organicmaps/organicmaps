import Foundation

fileprivate func IPAD() -> Bool { return UI_USER_INTERFACE_IDIOM() == .pad }

func L(_ key: String) -> String { return NSLocalizedString(key, comment: "") }

func alternative<T>(iPhone: T, iPad: T) -> T { return IPAD() ? iPad : iPhone }

func iPadSpecific(_ f: () -> Void) {
  if IPAD() {
    f()
  }
}

func iPhoneSpecific(_ f: () -> Void) {
  if !IPAD() {
    f()
  }
}

func toString(_ cls: AnyClass) -> String {
  return String(describing: cls)
}

func statusBarHeight() -> CGFloat {
  let statusBarSize = UIApplication.shared.statusBarFrame.size
  return min(statusBarSize.height, statusBarSize.width)
}
