import CarPlay

@objc(CarPlaySceneDelegate)
final class CarPlaySceneDelegate: UIResponder, CPTemplateApplicationSceneDelegate {
  func sceneDidBecomeActive(_ scene: UIScene) {
    MapsAppDelegate.theApp().sceneDidBecomeActive(scene)
  }

  func sceneWillResignActive(_ scene: UIScene) {
    MapsAppDelegate.theApp().sceneWillResignActive(scene)
  }

  func sceneWillEnterForeground(_ scene: UIScene) {
    MapsAppDelegate.theApp().sceneWillEnterForeground(scene)
  }

  func sceneDidEnterBackground(_ scene: UIScene) {
    MapsAppDelegate.theApp().sceneDidEnterBackground(scene)
  }

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
