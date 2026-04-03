@objc(MWMThemeManager)
final class ThemeManager: NSObject {
  private static let instance = ThemeManager()

  override private init() {
    super.init()
  }

  private func update(theme: MWMTheme) {
    updateSystemUserInterfaceStyle(theme)

    let actualTheme: MWMTheme = { theme in
      let isVehicleRouting = MWMRouter.isRoutingActive() && (MWMRouter.type() == .vehicle)
      switch theme {
      case .day: fallthrough
      case .vehicleDay: return isVehicleRouting ? .vehicleDay : .day
      case .night: fallthrough
      case .vehicleNight: return isVehicleRouting ? .vehicleNight : .night
      case .auto:
        let isDarkModeEnabled = UIScreen.main.traitCollection.userInterfaceStyle == .dark
        guard isVehicleRouting else { return isDarkModeEnabled ? .night : .day }
        return isDarkModeEnabled ? .vehicleNight : .vehicleDay
      @unknown default:
        fatalError()
      }
    }(theme)

    let nightMode = UIColor.isNightMode()
    let newNightMode: Bool = { theme in
      switch theme {
      case .day: fallthrough
      case .vehicleDay: return false
      case .night: fallthrough
      case .vehicleNight: return true
      case .auto: assertionFailure(); return false
      @unknown default:
        fatalError()
      }
    }(actualTheme)

    FrameworkHelper.setTheme(actualTheme)
    if nightMode != newNightMode || StyleManager.shared.hasTheme() == false {
      UIColor.setNightMode(newNightMode)
      if newNightMode {
        StyleManager.shared.setTheme(MainTheme(type: .dark, colors: NightColors(), fonts: Fonts()))
      } else {
        StyleManager.shared.setTheme(MainTheme(type: .light, colors: DayColors(), fonts: Fonts()))
      }
    }
  }

  @objc static func invalidate() {
    instance.update(theme: Settings.theme())
  }

  private func updateSystemUserInterfaceStyle(_ theme: MWMTheme) {
    let userInterfaceStyle: UIUserInterfaceStyle = { theme in
      switch theme {
      case .day: fallthrough
      case .vehicleDay: return .light
      case .night: fallthrough
      case .vehicleNight: return .dark
      case .auto: return .unspecified
      @unknown default:
        fatalError()
      }
    }(theme)
    UIApplication.shared.delegate?.window??.overrideUserInterfaceStyle = userInterfaceStyle
  }
}
