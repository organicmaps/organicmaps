import UIKit

@objc(MainSceneDelegate)
final class MainSceneDelegate: UIResponder, UIWindowSceneDelegate {
  var window: UIWindow?

  func scene(_ scene: UIScene,
             willConnectTo _: UISceneSession,
             options connectionOptions: UIScene.ConnectionOptions) {
    guard let windowScene = scene as? UIWindowScene,
          let sceneWindow = windowScene.windows.first
    else {
      assertionFailure("Main UIWindowScene is missing the storyboard-loaded window.")
      return
    }
    window = sceneWindow
    MapsAppDelegate.theApp().window = sceneWindow

    // Route cold-launch payloads delivered via the scene. Pre-UIScene iOS received these via
    // UIApplication launchOptions / application:openURL: and deferred deep-link handling until
    // MapViewController was ready. Mirror that deferred flow by seeding DeepLinkHandler here;
    // MapViewController.viewDidLoad drains it via handleDeepLinkAndReset().
    if !connectionOptions.urlContexts.isEmpty {
      if let launchURL = connectionOptions.urlContexts.first?.url {
        DeepLinkHandler.shared.applicationDidFinishLaunching([.url: launchURL])
      }
      self.scene(scene, openURLContexts: connectionOptions.urlContexts)
    }
    for userActivity in connectionOptions.userActivities {
      self.scene(scene, continue: userActivity)
    }
    if let shortcutItem = connectionOptions.shortcutItem {
      self.windowScene(windowScene, performActionFor: shortcutItem, completionHandler: { _ in })
    }
  }

  // MARK: - Lifecycle forwarding

  func sceneDidBecomeActive(_: UIScene) {
    MapsAppDelegate.theApp().applicationDidBecomeActive(.shared)
  }

  func sceneWillResignActive(_: UIScene) {
    MapsAppDelegate.theApp().applicationWillResignActive(.shared)
  }

  func sceneWillEnterForeground(_: UIScene) {
    MapsAppDelegate.theApp().applicationWillEnterForeground(.shared)
  }

  func sceneDidEnterBackground(_: UIScene) {
    MapsAppDelegate.theApp().applicationDidEnterBackground(.shared)
  }

  // MARK: - URL / user activity / shortcut forwarding

  func scene(_: UIScene, openURLContexts URLContexts: Set<UIOpenURLContext>) {
    for context in URLContexts {
      _ = MapsAppDelegate.theApp().application(.shared,
                                               open: context.url,
                                               options: context.options.asDictionary)
    }
  }

  func scene(_: UIScene, continue userActivity: NSUserActivity) {
    _ = MapsAppDelegate.theApp().application(.shared,
                                             continue: userActivity,
                                             restorationHandler: { _ in })
  }

  func windowScene(_: UIWindowScene,
                   performActionFor shortcutItem: UIApplicationShortcutItem,
                   completionHandler: @escaping (Bool) -> Void) {
    MapsAppDelegate.theApp().application(.shared,
                                         performActionFor: shortcutItem,
                                         completionHandler: completionHandler)
  }
}

private extension UIScene.OpenURLOptions {
  var asDictionary: [UIApplication.OpenURLOptionsKey: Any] {
    var options: [UIApplication.OpenURLOptionsKey: Any] = [
      .openInPlace: openInPlace,
    ]
    if let sourceApplication {
      options[.sourceApplication] = sourceApplication
    }
    if let annotation {
      options[.annotation] = annotation
    }
    if let eventAttribution {
      options[.eventAttribution] = eventAttribution
    }
    return options
  }
}
