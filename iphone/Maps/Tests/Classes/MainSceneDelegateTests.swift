@testable import Organic_Maps__Debug_
import UIKit
import XCTest

final class MainSceneDelegateTests: XCTestCase {
  private var manifest: [String: Any] {
    get throws {
      try XCTUnwrap(Bundle.main.object(forInfoDictionaryKey: "UIApplicationSceneManifest") as? [String: Any])
    }
  }

  /// OMaps.plist references the delegate by its bare class name, which the Swift class provides through
  /// a stable @objc name (the Debug product's Swift module is mangled to Organic_Maps__Debug_).
  func testManifestPhoneSceneDelegateResolvesToMainSceneDelegate() throws {
    let configurations = try XCTUnwrap(try manifest["UISceneConfigurations"] as? [String: [[String: String]]])
    let phone = try XCTUnwrap(configurations["UIWindowSceneSessionRoleApplication"]?.first)
    let className = try XCTUnwrap(phone["UISceneDelegateClassName"])
    XCTAssertTrue(NSClassFromString(className) === MainSceneDelegate.self)
    XCTAssertTrue(class_conformsToProtocol(MainSceneDelegate.self, UIWindowSceneDelegate.self))
  }

  /// One Drape engine and one navigation stack cannot be hosted by two phone windows, so the app must
  /// not offer multi-window support; the CarPlay scene does not need it.
  func testManifestDoesNotEnableMultipleScenes() throws {
    XCTAssertNotEqual(try manifest["UIApplicationSupportsMultipleScenes"] as? Bool, true)
  }
}
