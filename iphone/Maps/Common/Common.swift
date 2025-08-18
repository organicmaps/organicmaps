import Foundation

var isiPad: Bool {
  if #available(iOS 14.0, *), ProcessInfo.processInfo.isiOSAppOnMac {
    return true
  }
  return UIDevice.current.userInterfaceIdiom == .pad
}

func L(_ key: String) -> String { return NSLocalizedString(key, comment: "") }

func L(_ key: String, languageCode: String) -> String {
  guard let path = Bundle.main.path(forResource: languageCode, ofType: "lproj"),
        let bundle = Bundle(path: path) else {
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
  return String(describing: cls)
}

func statusBarHeight() -> CGFloat {
  let statusBarSize = UIApplication.shared.statusBarFrame.size
  return min(statusBarSize.height, statusBarSize.width)
}

func LOG(_ level: LogLevel,
         _ message: @autoclosure () -> Any,
         functionName: StaticString = #function,
         fileName: StaticString = #file,
         lineNumber: UInt = #line) {
  if (Logger.canLog(level)) {
    let shortFileName = URL(string: "\(fileName)")?.lastPathComponent ?? ""
    let formattedMessage = "\(shortFileName):\(lineNumber) \(functionName): \(message())"
    Logger.log(level, message: formattedMessage)
  }
}

struct Weak<T> where T: AnyObject {
  weak var value: T?
}
