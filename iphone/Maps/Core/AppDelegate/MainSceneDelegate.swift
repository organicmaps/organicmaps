import UIKit

@objc(MainSceneDelegate)
final class MainSceneDelegate: UIResponder, UIWindowSceneDelegate {
  var window: UIWindow?

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
    // system does not auto-load a duplicate MapViewController from the storyboard.
    let sceneWindow = UIWindow(windowScene: windowScene)
    sceneWindow.rootViewController = app.mainNavigationController
    window = sceneWindow
    app.window = sceneWindow
    sceneWindow.makeKeyAndVisible()

    // CarPlay scene can connect before the main phone scene; now that the window is ready, attach the
    // shared map view to the CarPlay controller if it was deferred.
    CarPlayService.shared.attachMapIfNeeded()

    // Route cold-launch payloads delivered via the scene. Pre-UIScene iOS received these via
    // UIApplication launchOptions / application:openURL: and deferred deep-link handling until
    // MapViewController was ready. Mirror that deferred flow by seeding DeepLinkHandler here;
    // MapViewController.viewDidLoad drains it via handleDeepLinkAndReset().
    if !connectionOptions.urlContexts.isEmpty {
      // iOS delivers a single URL on a cold launch, so the arbitrary Set order is not a concern here.
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

  /// UIWindowSceneDelegate callbacks are per-scene. While CarPlay is connected it keeps its own scene
  /// foregrounded, so the phone window scene backgrounding/resigning (lock screen, app switcher) must
  /// not drive the app-wide background path — that would pause the render loop and location updates the
  /// CarPlay session still needs against the same shared map. Forward only when CarPlay is inactive.
  private func forwardLifecycleEventUnlessCarPlayActive(_ forward: (MapsAppDelegate) -> Void) {
    guard !CarPlayService.shared.isCarplayActivated else { return }
    forward(MapsAppDelegate.theApp())
  }

  func sceneDidBecomeActive(_: UIScene) {
    forwardLifecycleEventUnlessCarPlayActive { $0.applicationDidBecomeActive(.shared) }
  }

  func sceneWillResignActive(_: UIScene) {
    forwardLifecycleEventUnlessCarPlayActive { $0.applicationWillResignActive(.shared) }
  }

  func sceneWillEnterForeground(_: UIScene) {
    forwardLifecycleEventUnlessCarPlayActive { $0.applicationWillEnterForeground(.shared) }
  }

  func sceneDidEnterBackground(_: UIScene) {
    forwardLifecycleEventUnlessCarPlayActive { $0.applicationDidEnterBackground(.shared) }
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
