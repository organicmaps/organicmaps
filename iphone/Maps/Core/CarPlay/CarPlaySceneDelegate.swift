import CarPlay

@objc(CarPlaySceneDelegate)
final class CarPlaySceneDelegate: UIResponder, CPTemplateApplicationSceneDelegate {
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
