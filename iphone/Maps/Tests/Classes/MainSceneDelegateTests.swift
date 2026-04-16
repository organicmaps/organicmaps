@testable import Organic_Maps__Debug_
import UIKit
import XCTest

final class MainSceneDelegateTests: XCTestCase {
  /// OMaps.plist references "MainSceneDelegate" as UISceneDelegateClassName. The Swift class
  /// needs a stable @objc name for the Obj-C runtime to resolve it, because the Debug product's
  /// Swift module is mangled to Organic_Maps__Debug_.
  func testRuntimeClassNameMatchesPlist() {
    XCTAssertEqual(NSStringFromClass(MainSceneDelegate.self), "MainSceneDelegate")
  }

  func testConformsToUIWindowSceneDelegate() {
    XCTAssertTrue(MainSceneDelegate() is UIWindowSceneDelegate)
  }

  // Once UIApplicationSceneManifest declares UIWindowSceneSessionRoleApplication, UIKit stops
  // calling the matching UIApplicationDelegate methods on MapsAppDelegate. The scene delegate
  // must implement the scene-side equivalents and forward them; missing any of these regresses
  // Framework::Enter{Fore,Back}ground, location pause/resume, route saving, deep-link resets,
  // and the rate-us prompt.
  func testRespondsToSceneLifecycleSelectors() {
    let delegate = MainSceneDelegate()
    XCTAssertTrue(delegate.responds(to: #selector(MainSceneDelegate.sceneDidBecomeActive(_:))))
    XCTAssertTrue(delegate.responds(to: #selector(MainSceneDelegate.sceneWillResignActive(_:))))
    XCTAssertTrue(delegate.responds(to: #selector(MainSceneDelegate.sceneWillEnterForeground(_:))))
    XCTAssertTrue(delegate.responds(to: #selector(MainSceneDelegate.sceneDidEnterBackground(_:))))
  }

  /// mapswithme:// deep links, imported GPX/KML files, Spotlight/Handoff universal links, and
  /// Home Screen quick actions are delivered through UIScene once scenes are adopted. Each
  /// entry point needs its scene-side forwarder on MainSceneDelegate.
  func testRespondsToLaunchRoutingSelectors() {
    let delegate = MainSceneDelegate()
    XCTAssertTrue(delegate.responds(to: Selector(("scene:openURLContexts:"))))
    XCTAssertTrue(delegate.responds(to: Selector(("scene:continueUserActivity:"))))
    XCTAssertTrue(delegate.responds(to: Selector(("windowScene:performActionForShortcutItem:completionHandler:"))))
  }
}
