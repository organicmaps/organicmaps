import Foundation

var isIPad: Bool { return UI_USER_INTERFACE_IDIOM() == .pad }

func L(_ key: String) -> String { return NSLocalizedString(key, comment: "") }

func alternative<T>(iPhone: T, iPad: T) -> T { return isIPad ? iPad : iPhone }

func iPadSpecific(_ f: () -> Void) {
  if isIPad {
    f()
  }
}

func iPhoneSpecific(_ f: () -> Void) {
  if !isIPad {
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
