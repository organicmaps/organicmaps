@testable import Organic_Maps__Debug_
import UIKit
import XCTest

final class MainSceneDelegateTests: XCTestCase {
  override func tearDown() {
    DeepLinkHandler.shared.reset()
    super.tearDown()
  }

  /// OMaps.plist references "MainSceneDelegate" as UISceneDelegateClassName. The Swift class
  /// needs a stable @objc name for the Obj-C runtime to resolve it, because the Debug product's
  /// Swift module is mangled to Organic_Maps__Debug_.
  func testRuntimeClassNameMatchesPlist() {
    XCTAssertEqual(NSStringFromClass(MainSceneDelegate.self), "MainSceneDelegate")
  }

  func testRespondsToForwardingSelectors() {
    let delegate = MainSceneDelegate()
    XCTAssertTrue(delegate.responds(to: #selector(MainSceneDelegate.sceneDidBecomeActive(_:))))
    XCTAssertTrue(delegate.responds(to: #selector(MainSceneDelegate.sceneWillResignActive(_:))))
    XCTAssertTrue(delegate.responds(to: #selector(MainSceneDelegate.sceneWillEnterForeground(_:))))
    XCTAssertTrue(delegate.responds(to: #selector(MainSceneDelegate.sceneDidEnterBackground(_:))))
    XCTAssertTrue(delegate.responds(to: #selector(MainSceneDelegate.sceneDidDisconnect(_:))))
    XCTAssertTrue(delegate.responds(to: #selector(MainSceneDelegate.scene(_:openURLContexts:))))
    XCTAssertTrue(delegate.responds(to: #selector(MainSceneDelegate.scene(_:continue:))))
    XCTAssertTrue(delegate.responds(to: #selector(MainSceneDelegate.windowScene(_:performActionFor:completionHandler:))))
  }

  func testColdUniversalLinkIsQueuedUntilMapIsReady() throws {
    let handler = DeepLinkHandler.shared
    try handler.prepareForColdLaunch(universalLinks: [XCTUnwrap(URL(string: "https://omaps.app/AbCd/Test"))])

    XCTAssertTrue(handler.hasPendingColdLaunchDeepLink)
    XCTAssertFalse(handler.isLaunchedByDeeplink)
    XCTAssertTrue(handler.isLaunchedByUniversalLink)
    XCTAssertEqual(handler.url?.absoluteString, "om://AbCd/Test")
  }

  func testApplicationSupportsPhoneAndCarPlayScenesSimultaneously() throws {
    let manifest = try XCTUnwrap(Bundle.main.object(forInfoDictionaryKey: "UIApplicationSceneManifest") as? [String: Any])
    XCTAssertEqual(manifest["UIApplicationSupportsMultipleScenes"] as? Bool, true)
  }
}
