@objc(MWMThemeManager)
final class ThemeManager: NSObject {
  private static let instance = ThemeManager()
  private var isNightMode = false

  override private init() {
    super.init()
    NotificationCenter.default.addObserver(self,
                                           selector: #selector(contentSizeCategoryDidChange),
                                           name: UIContentSizeCategory.didChangeNotification,
                                           object: nil)
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

    // The map style family (vehicle / outdoors / default) is resolved by the core from the routing
    // and outdoors-layer state; here we resolve only light vs dark.
    let actualTheme: MWMTheme = { theme in
      switch theme {
      case .day, .vehicleDay: return .day
      case .night, .vehicleNight: return .night
      case .auto: return UIScreen.main.traitCollection.userInterfaceStyle == .dark ? .night : .day
      @unknown default:
        fatalError()
      }
    }(effectivePreference)

    let newNightMode = actualTheme == .night

    FrameworkHelper.setTheme(actualTheme)

    if !StyleManager.shared.hasTheme() {
      isNightMode = newNightMode
      StyleManager.shared.setTheme(MainTheme())
    } else if isNightMode != newNightMode {
      // Re-apply styles for non-dynamic properties (CGColor, themed images).
      isNightMode = newNightMode
      StyleManager.shared.update()
    }
  }

  @objc static func invalidate() {
    // On macOS, UIKit keeps delivering appearance/trait changes while the app terminates,
    // after applicationWillTerminate: has destroyed the C++ Framework. Skip theming to avoid
    // calling GetFramework() on a destroyed singleton (which trips its CHECK and aborts).
    guard !FrameworkHelper.isFrameworkDestroyed() else { return }
    instance.update(theme: Settings.theme())
  }

  @objc private func contentSizeCategoryDidChange() {
    StyleManager.shared.update()
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
