import UIKit

@objc(MainSceneDelegate)
final class MainSceneDelegate: UIResponder, UIWindowSceneDelegate {
  var window: UIWindow? {
    get { MapsAppDelegate.theApp().window }
    set { MapsAppDelegate.theApp().window = newValue }
  }

  func scene(_ scene: UIScene,
             willConnectTo _: UISceneSession,
             options connectionOptions: UIScene.ConnectionOptions) {
    guard let windowScene = scene as? UIWindowScene else {
      assertionFailure("Main scene is not a UIWindowScene.")
      return
    }
    let app = MapsAppDelegate.theApp()

    // Build the window around the single shared root navigation controller. On a CarPlay-first cold
    // launch CarPlayService may have already created it (so the map could render on the head unit);
    // reusing it here keeps one MapViewController and one Drape engine across both scenes instead of
    // instantiating a second map. We create the window explicitly (no UISceneStoryboardFile) so the
    // system does not auto-load a duplicate MapViewController from the storyboard. The manifest does
    // not enable multiple scenes, so this is the only phone window; a reconnect after a disconnect
    // re-hosts the same navigation controller.
    let sceneWindow = UIWindow(windowScene: windowScene)
    window = sceneWindow
    sceneWindow.rootViewController = app.mainNavigationController

    // File URLs must be imported synchronously while their security-scoped resources are available.
    // Keep the last non-file URL before showing the window because makeKeyAndVisible() can trigger the
    // map's appearance callbacks, and MapViewController.viewDidAppear handles the pending cold-launch link.
    // In practice the set always holds a single URL: iOS delivers one link per call.
    for context in connectionOptions.urlContexts {
      if context.url.isFileURL {
        _ = DeepLinkHandler.shared.applicationDidOpenUrl(context.url, openInPlace: context.options.openInPlace)
      } else {
        DeepLinkHandler.shared.prepareForColdLaunch(url: context.url)
      }
    }
    for userActivity in connectionOptions.userActivities
      where userActivity.activityType == NSUserActivityTypeBrowsingWeb {
      if let url = userActivity.webpageURL {
        DeepLinkHandler.shared.prepareForColdLaunch(universalLink: url)
      }
    }
    // The launch-time invalidation runs before an app window exists, so refresh again to apply its style.
    ThemeManager.invalidate()
    sceneWindow.makeKeyAndVisible()

    // Route the remaining cold-launch payloads delivered via the scene.
    for userActivity in connectionOptions.userActivities
      where userActivity.activityType != NSUserActivityTypeBrowsingWeb || userActivity.webpageURL == nil {
      self.scene(scene, continue: userActivity)
    }
    if let shortcutItem = connectionOptions.shortcutItem {
      self.windowScene(windowScene, performActionFor: shortcutItem, completionHandler: { _ in })
    }
  }

  func sceneDidDisconnect(_: UIScene) {
    // The shared navigation controller and the map outlive the window (CarPlay may still show it).
    window = nil
  }

  // Activation and background transitions are handled app-wide by MapsAppDelegate through the
  // UIApplication notifications; forwarding them per scene would background the framework while
  // CarPlay keeps the app in the foreground.

  // MARK: - URL / user activity / shortcut forwarding

  func scene(_: UIScene, openURLContexts URLContexts: Set<UIOpenURLContext>) {
    for context in URLContexts {
      _ = DeepLinkHandler.shared.applicationDidOpenUrl(context.url, openInPlace: context.options.openInPlace)
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
}
