@objc(MWMThemeManager)
final class ThemeManager: NSObject {

  private static let autoUpdatesInterval: TimeInterval = 30 * 60 // 30 minutes in seconds

  private static let instance = ThemeManager()
  private weak var timer: Timer?

  private override init() { super.init() }

  private func update(theme: MWMTheme) {
    let actualTheme: MWMTheme = { theme in
      guard theme == .auto else { return theme }
      guard MWMRouter.isRoutingActive() else { return .day }
      switch MWMFrameworkHelper.daytime() {
      case .day: return .day
      case .night: return .night
      }
    }(theme)

    let nightMode = UIColor.isNightMode()
    let newNightMode: Bool = { theme in
      switch theme {
      case .day: return false
      case .night: return true
      case .auto: assert(false); return false
      }
    }(actualTheme)

    MWMFrameworkHelper.setTheme(actualTheme)
    if nightMode != newNightMode {
      UIColor.setNightMode(newNightMode)
      (UIViewController.topViewController() as! MWMController).mwm_refreshUI()
    }
  }

  static func invalidate() {
    instance.update(theme: MWMSettings.theme())
  }

  static var autoUpdates: Bool {
    get {
      return instance.timer != nil
    }
    set {
      if newValue {
        instance.timer = Timer.scheduledTimer(timeInterval: autoUpdatesInterval, target: self, selector: #selector(invalidate), userInfo: nil, repeats: true)
      } else {
        instance.timer?.invalidate()
      }
      invalidate()
    }
  }
}
