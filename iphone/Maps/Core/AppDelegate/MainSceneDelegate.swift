import UIKit

@objc(MainSceneDelegate)
final class MainSceneDelegate: UIResponder, UIWindowSceneDelegate {
  var window: UIWindow?

  func scene(_ scene: UIScene,
             willConnectTo session: UISceneSession,
             options connectionOptions: UIScene.ConnectionOptions) {
    guard let windowScene = scene as? UIWindowScene else {
      assertionFailure("Main scene is not a UIWindowScene.")
      return
    }
    let app = MapsAppDelegate.theApp()
    guard app.window == nil else {
      // The app shares one map renderer and navigation stack between the phone and CarPlay.
      // Keep CarPlay's separate scene, but reject an additional phone window.
      UIApplication.shared.requestSceneSessionDestruction(session, options: nil)
      return
    }

    // Build the window around the single shared root navigation controller. On a CarPlay-first cold
    // launch CarPlayService may have already created it (so the map could render on the head unit);
    // reusing it here keeps one MapViewController and one Drape engine across both scenes instead of
    // instantiating a second map. We create the window explicitly (no UISceneStoryboardFile) so the
    // system does not auto-load a duplicate MapViewController from the storyboard.
    let sceneWindow = UIWindow(windowScene: windowScene)
    window = sceneWindow
    app.window = sceneWindow
    sceneWindow.rootViewController = app.mainNavigationController

    // File URLs must be imported synchronously while their security-scoped resources are available.
    // Queue the launch deep link before showing the window because makeKeyAndVisible() can trigger
    // the map's appearance callbacks, and MapViewController.viewDidAppear consumes that link.
    let launchURLContexts = sorted(connectionOptions.urlContexts)
    for context in launchURLContexts where context.url.isFileURL {
      _ = app.handleOpenURL(context.url, openInPlace: context.options.openInPlace)
    }
    let launchUniversalLinks = connectionOptions.userActivities
      .filter { $0.activityType == NSUserActivityTypeBrowsingWeb }
      .compactMap(\.webpageURL)
      .sorted { $0.absoluteString < $1.absoluteString }
    var launchDeepLinks: [URL] = []
    for context in launchURLContexts where !context.url.isFileURL {
      guard !CarPlayService.shared.handleOpenCarPlayURL(context.url) else { continue }
      launchDeepLinks.append(context.url)
    }
    DeepLinkHandler.shared.prepareForColdLaunch(
      urls: launchDeepLinks,
      universalLinks: launchUniversalLinks
    )
    ThemeManager.invalidate()
    sceneWindow.makeKeyAndVisible()

    // CarPlay may connect before the phone window. Reconcile the latest phone/car preference now
    // that the device display is available.
    CarPlayService.shared.phoneSceneDidConnect()

    // Route the remaining cold-launch payloads delivered via the scene.
    for userActivity in connectionOptions.userActivities
      where userActivity.activityType != NSUserActivityTypeBrowsingWeb || userActivity.webpageURL == nil {
      self.scene(scene, continue: userActivity)
    }
    if let shortcutItem = connectionOptions.shortcutItem {
      self.windowScene(windowScene, performActionFor: shortcutItem, completionHandler: { _ in })
    }
  }

  func sceneDidDisconnect(_ scene: UIScene) {
    guard let sceneWindow = window, sceneWindow.windowScene === scene else { return }

    CarPlayService.shared.phoneSceneDidDisconnect()
    let app = MapsAppDelegate.theApp()
    if app.window === sceneWindow {
      app.window = nil
    }
    window = nil
  }

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

  // MARK: - URL / user activity / shortcut forwarding

  func scene(_: UIScene, openURLContexts URLContexts: Set<UIOpenURLContext>) {
    let app = MapsAppDelegate.theApp()
    for context in sorted(URLContexts) {
      if CarPlayService.shared.handleOpenCarPlayURL(context.url) {
        continue
      }
      _ = app.handleOpenURL(context.url, openInPlace: context.options.openInPlace)
    }
  }

  func scene(_: UIScene, continue userActivity: NSUserActivity) {
    _ = MapsAppDelegate.theApp().handleUserActivity(userActivity)
  }

  func windowScene(_: UIWindowScene,
                   performActionFor shortcutItem: UIApplicationShortcutItem,
                   completionHandler: @escaping (Bool) -> Void) {
    MapsAppDelegate.theApp().handleShortcutItem(shortcutItem, completionHandler: completionHandler)
  }

  private func sorted(_ URLContexts: Set<UIOpenURLContext>) -> [UIOpenURLContext] {
    URLContexts.sorted { lhs, rhs in
      lhs.url.absoluteString < rhs.url.absoluteString
    }
  }
}
