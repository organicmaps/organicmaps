@objc(MWMThemeManager)
final class ThemeManager: NSObject {

  private static let autoUpdatesInterval: TimeInterval = 30 * 60;

  private static let instance = ThemeManager()
  private weak var timer: Timer?

  private override init() { super.init() }

  private func update(theme: MWMTheme) {
    let actualTheme = { theme -> MWMTheme in
      guard theme == .auto else { return theme }
      guard MWMRouter.isRoutingActive() else { return .day }
      switch MWMFrameworkHelper.daytime() {
      case .day: return .day
      case .night: return .night
      }
    }(theme)
    MWMFrameworkHelper.setTheme(actualTheme)
    let nightMode = UIColor.isNightMode()
    var newNightMode = false
    switch actualTheme {
    case .day: break
    case .night: newNightMode = true
    case .auto: assert(false)
    }
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
