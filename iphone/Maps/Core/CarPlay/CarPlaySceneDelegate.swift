import CarPlay

@objc(CarPlaySceneDelegate)
final class CarPlaySceneDelegate: UIResponder, CPTemplateApplicationSceneDelegate {
  /// Activation and background transitions are handled app-wide by MapsAppDelegate through the
  /// UIApplication notifications, so this delegate only tracks the CarPlay connection itself.
  func templateApplicationScene(_: CPTemplateApplicationScene,
                                didConnect interfaceController: CPInterfaceController,
                                to window: CPWindow) {
    CarPlayService.shared.setup(window: window, interfaceController: interfaceController)
  }

  func templateApplicationScene(_: CPTemplateApplicationScene,
                                didDisconnect _: CPInterfaceController,
                                from _: CPWindow) {
    CarPlayService.shared.destroy()
  }
}
