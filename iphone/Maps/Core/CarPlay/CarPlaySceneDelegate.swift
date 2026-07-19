import CarPlay

@objc(CarPlaySceneDelegate)
final class CarPlaySceneDelegate: UIResponder,
  CPTemplateApplicationSceneDelegate,
  CPTemplateApplicationDashboardSceneDelegate {
  func sceneDidBecomeActive(_ scene: UIScene) {
    MapsAppDelegate.theApp().sceneDidBecomeActive(scene)
    CarPlayService.shared.sceneDidBecomeActive(scene)
  }

  func sceneWillResignActive(_ scene: UIScene) {
    MapsAppDelegate.theApp().sceneWillResignActive(scene)
  }

  func sceneWillEnterForeground(_ scene: UIScene) {
    MapsAppDelegate.theApp().sceneWillEnterForeground(scene)
    CarPlayService.shared.sceneWillEnterForeground(scene)
  }

  func sceneDidEnterBackground(_ scene: UIScene) {
    MapsAppDelegate.theApp().sceneDidEnterBackground(scene)
  }

  func scene(_: UIScene, openURLContexts URLContexts: Set<UIOpenURLContext>) {
    for context in URLContexts {
      _ = CarPlayService.shared.handleOpenCarPlayURL(context.url)
    }
  }

  func templateApplicationScene(_ scene: CPTemplateApplicationScene,
                                didConnect interfaceController: CPInterfaceController,
                                to window: CPWindow) {
    CarPlayService.shared.setup(window: window, interfaceController: interfaceController)
    reconcileMapIfAlreadyActive(scene)
  }

  func templateApplicationScene(_: CPTemplateApplicationScene,
                                didDisconnect _: CPInterfaceController,
                                from window: CPWindow) {
    CarPlayService.shared.destroy(window: window)
  }

  func templateApplicationDashboardScene(_ scene: CPTemplateApplicationDashboardScene,
                                         didConnect dashboardController: CPDashboardController,
                                         to window: UIWindow) {
    CarPlayService.shared.setupDashboard(scene: scene, window: window, dashboardController: dashboardController)
    reconcileMapIfAlreadyActive(scene)
  }

  func templateApplicationDashboardScene(_: CPTemplateApplicationDashboardScene,
                                         didDisconnect _: CPDashboardController,
                                         from window: UIWindow) {
    CarPlayService.shared.destroyDashboard(window: window)
  }

  private func reconcileMapIfAlreadyActive(_ scene: UIScene) {
    guard scene.activationState == .foregroundActive else { return }
    CarPlayService.shared.sceneDidBecomeActive(scene)
  }
}
