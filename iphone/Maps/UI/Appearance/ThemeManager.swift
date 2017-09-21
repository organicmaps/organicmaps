@objc(MWMThemeManager)
final class ThemeManager: NSObject {

  private static let autoUpdatesInterval: TimeInterval = 30 * 60 // 30 minutes in seconds

  private static let instance = ThemeManager()
  private weak var timer: Timer?

  private override init() { super.init() }

  private func update(theme: MWMTheme) {
    let actualTheme: MWMTheme = { theme in
      let isVehicleRouting = MWMRouter.isRoutingActive() && (MWMRouter.type() == .vehicle)
      switch theme {
      case .day: fallthrough
      case .vehicleDay: return isVehicleRouting ? .vehicleDay : .day
      case .night: fallthrough
      case .vehicleNight: return isVehicleRouting ? .vehicleNight : .night
      case .auto:
        guard isVehicleRouting else { return .day }
        switch MWMFrameworkHelper.daytime() {
        case .day: return .vehicleDay
        case .night: return .vehicleNight
        }
      }
    }(theme)

    let nightMode = UIColor.isNightMode()
    let newNightMode: Bool = { theme in
      switch theme {
      case .day: fallthrough
      case .vehicleDay: return false
      case .night: fallthrough
      case .vehicleNight: return true
      case .auto: assert(false); return false
      }
    }(actualTheme)

    MWMFrameworkHelper.setTheme(actualTheme)
    if nightMode != newNightMode {
      UIColor.setNightMode(newNightMode)
      (UIViewController.topViewController() as! MWMController).mwm_refreshUI()
    }
  }

  @objc static func invalidate() {
    instance.update(theme: MWMSettings.theme())
  }

  @objc static var autoUpdates: Bool {
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
