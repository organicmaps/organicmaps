import Foundation

@objcMembers
final class AppConstants: NSObject {
  private override init() {}
  static let defaultAnimationDuration: TimeInterval = 0.3
  static let fastAnimationDuration: TimeInterval = 0.15
  // The last 5% are left for applying diffs.
  static let maxProgress: Float = 0.95
}

var isiPad: Bool {
  if ProcessInfo.processInfo.isiOSAppOnMac {
    return true
  }
  return UIDevice.current.userInterfaceIdiom == .pad
}

func L(_ key: String) -> String { NSLocalizedString(key, comment: "") }

func L(_ key: String, languageCode: String) -> String {
  guard let path = Bundle.main.path(forResource: languageCode, ofType: "lproj"),
        let bundle = Bundle(path: path)
  else {
    LOG(.warning, "Localization bundle not found for language code: \(languageCode)")
    return L(key)
  }
  return NSLocalizedString(key, bundle: bundle, comment: "")
}

func alternative<T>(iPhone: T, iPad: T) -> T { isiPad ? iPad : iPhone }

func iPadSpecific(_ f: () -> Void) {
  if isiPad {
    f()
  }
}

func iPhoneSpecific(_ f: () -> Void) {
  if !isiPad {
    f()
  }
}

func toString(_ cls: AnyClass) -> String {
  String(describing: cls)
}

func LOG(_ level: LogLevel,
         _ message: @autoclosure () -> Any,
         functionName: StaticString = #function,
         fileName: StaticString = #file,
         lineNumber: UInt = #line) {
  if Logger.canLog(level) {
    let shortFileName = URL(string: "\(fileName)")?.lastPathComponent ?? ""
    let formattedMessage = "\(shortFileName):\(lineNumber) \(functionName): \(message())"
    Logger.log(level, message: formattedMessage)
  }
}

struct Weak<T: AnyObject> {
  weak var value: T?
}
