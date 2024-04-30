import Foundation
import OSLog

private func IPAD() -> Bool { return UI_USER_INTERFACE_IDIOM() == .pad }

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

private let enableLoggingInRelease = true

func LOG(_ level: LogLevel,
         _ message: @autoclosure () -> Any,
         functionName: StaticString = #function,
         fileName: StaticString = #file,
         lineNumber: UInt = #line) {

  let shortFileName = URL(string: "\(fileName)")?.lastPathComponent ?? ""
  let formattedMessage = "\(shortFileName):\(lineNumber) \(functionName): \(message())"

  if #available(iOS 14.0, *), enableLoggingInRelease {
    os.Logger.logger.log(level: OSLogLevelFromLogLevel(level), "\(formattedMessage, privacy: .public)")
  } else if Logger.canLog(level) {
    Logger.log(level, message: formattedMessage)
  }
}

private func OSLogLevelFromLogLevel(_ level: LogLevel) -> OSLogType {
  switch level {
  case .error: return .error
  case .info: return .info
  case .debug: return .debug
  case .critical: return .fault
  case .warning: return .default
  @unknown default:
    fatalError()
  }
}

struct Weak<T> where T: AnyObject {
  weak var value: T?
}

@available(iOS 14.0, *)
private extension os.Logger {
  static let subsystem = Bundle.main.bundleIdentifier!
  static let logger = Logger(subsystem: subsystem, category: "OM")
}
