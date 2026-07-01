enum SettingsScreen {
  case profile
  case units
  case mobileInternet
  case powerManagement
  case bookmarksTextPlacement
  case appearance
  case voiceInstructions
  case drivingOptions
  case mapTiles
}

@objcMembers
final class SettingsBuilder: NSObject {
  static func buildRoot() -> UIViewController {
    let viewController = RootSettingsViewController()
    let presenter = RootSettingsPresenter(viewController: viewController)
    let interactor = RootSettingsInteractor()
    interactor.presenter = presenter
    presenter.interactor = interactor
    viewController.configure(interactor: interactor)
    return viewController
  }

  @nonobjc static func build(_ screen: SettingsScreen) -> UIViewController {
    switch screen {
    case .profile:
      return UIStoryboard(name: "Authorization", bundle: nil)
        .instantiateViewController(withIdentifier: StoryboardId.authorizationProfile)
    case .units:
      return buildUnits()
    case .mobileInternet:
      return buildMobileInternet()
    case .powerManagement:
      return buildPowerManagement()
    case .bookmarksTextPlacement:
      return buildBookmarksTextPlacement()
    case .appearance:
      return buildAppearance()
    case .voiceInstructions:
      return buildTTSSettings()
    case .drivingOptions:
      return buildDrivingOptions()
    case .mapTiles:
      return buildMapTiles()
    }
  }

  static func buildMapTiles() -> UIViewController {
    let viewController = MapTilesSettingsViewController()
    let presenter = MapTilesSettingsPresenter(viewController: viewController)
    let interactor = MapTilesSettingsInteractor()
    interactor.presenter = presenter
    viewController.configure(interactor: interactor)
    return viewController
  }

  static func buildMobileInternet() -> UIViewController {
    let viewController = MobileInternetSettingsViewController()
    let presenter = MobileInternetSettingsPresenter(viewController: viewController)
    let interactor = MobileInternetSettingsInteractor()
    interactor.presenter = presenter
    viewController.configure(interactor: interactor)
    return viewController
  }

  static func buildUnits() -> UIViewController {
    let viewController = UnitsSettingsViewController()
    let presenter = UnitsSettingsPresenter(viewController: viewController)
    let interactor = UnitsSettingsInteractor()
    interactor.presenter = presenter
    viewController.configure(interactor: interactor)
    return viewController
  }

  static func buildPowerManagement() -> UIViewController {
    let viewController = PowerManagementSettingsViewController()
    let presenter = PowerManagementSettingsPresenter(viewController: viewController)
    let interactor = PowerManagementSettingsInteractor()
    interactor.presenter = presenter
    viewController.configure(interactor: interactor)
    return viewController
  }

  static func buildDrivingOptions() -> UIViewController {
    let viewController = RoutingOptionsSettingsViewController()
    let presenter = RoutingOptionsSettingsPresenter(viewController: viewController)
    let interactor = RoutingOptionsSettingsInteractor()
    interactor.presenter = presenter
    viewController.configure(interactor: interactor)
    return viewController
  }

  static func buildBookmarksTextPlacement() -> UIViewController {
    let viewController = BookmarksTextPlacementSettingsViewController()
    let presenter = BookmarksTextPlacementSettingsPresenter(viewController: viewController)
    let interactor = BookmarksTextPlacementSettingsInteractor()
    interactor.presenter = presenter
    viewController.configure(interactor: interactor)
    return viewController
  }

  static func buildAppearance() -> UIViewController {
    let viewController = AppearanceSettingsViewController()
    let presenter = AppearanceSettingsPresenter(viewController: viewController)
    let interactor = AppearanceSettingsInteractor()
    interactor.presenter = presenter
    viewController.configure(interactor: interactor)
    return viewController
  }

  static func buildTTSSettings() -> UIViewController {
    let viewController = TTSSettingsViewController()
    let presenter = TTSSettingsPresenter(viewController: viewController)
    let interactor = TTSSettingsInteractor()
    interactor.presenter = presenter
    viewController.configure(interactor: interactor)
    return viewController
  }

  static func buildTTSLanguage() -> UIViewController {
    let viewController = TTSLanguageSettingsViewController()
    let presenter = TTSLanguageSettingsPresenter(viewController: viewController)
    let interactor = TTSLanguageSettingsInteractor()
    interactor.presenter = presenter
    viewController.configure(interactor: interactor)
    return viewController
  }
}

// TODO: Remove when the OSMAuthorization view controllers will be rewritten on Swift.
private enum StoryboardId {
  static let authorizationProfile = "AuthorizationLoginViewController"
}
