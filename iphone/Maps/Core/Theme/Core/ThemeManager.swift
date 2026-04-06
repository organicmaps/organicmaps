@objc(MWMThemeManager)
final class ThemeManager: NSObject {
  private static let instance = ThemeManager()
  private var isNightMode = false

  override private init() {
    super.init()
  }

  private func update(theme: MWMTheme) {
    // CarPlay may override the user preference with its own light/dark style.
    var effectivePreference = theme
    if CarPlayService.shared.isCarplayActivated {
      let carPlayStyle = CarPlayService.shared.interfaceStyle()
      switch carPlayStyle {
      case .light: effectivePreference = .day
      case .dark: effectivePreference = .night
      default: break
      }
    }

    updateSystemUserInterfaceStyle(effectivePreference)

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
    }(effectivePreference)

    let newNightMode = actualTheme == .night || actualTheme == .vehicleNight

    FrameworkHelper.setTheme(actualTheme)

    if !StyleManager.shared.hasTheme() {
      isNightMode = newNightMode
      StyleManager.shared.setTheme(MainTheme(fonts: Fonts()))
    } else if isNightMode != newNightMode {
      // Re-apply styles for non-dynamic properties (CGColor, themed images).
      isNightMode = newNightMode
      StyleManager.shared.update()
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
