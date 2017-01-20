import Foundation

fileprivate func IPAD() -> Bool { return UI_USER_INTERFACE_IDIOM() == .pad }

func L(_ key: String) -> String { return NSLocalizedString(key, comment: "") }

func val<T>(iPhone: T, iPad: T) -> T { return IPAD() ? iPad : iPhone }

func iPadSpecific( _ f: () -> Void) {
  if IPAD() {
    f()
  }
}
func iPhoneSpecific( _ f: () -> Void) {
  if !IPAD() {
    f()
  }
}
