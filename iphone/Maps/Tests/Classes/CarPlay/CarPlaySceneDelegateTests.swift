import CarPlay
@testable import Organic_Maps__Debug_
import XCTest

final class CarPlaySceneDelegateTests: XCTestCase {
  /// OMaps.plist references the delegate by its bare class name, which the Swift class provides through
  /// a stable @objc name (the Debug product's Swift module is mangled to Organic_Maps__Debug_).
  func testManifestCarPlaySceneDelegateResolvesToCarPlaySceneDelegate() throws {
    let manifest = try XCTUnwrap(Bundle.main.object(forInfoDictionaryKey: "UIApplicationSceneManifest") as? [String: Any])
    let configurations = try XCTUnwrap(manifest["UISceneConfigurations"] as? [String: [[String: String]]])
    let carPlay = try XCTUnwrap(configurations["CPTemplateApplicationSceneSessionRoleApplication"]?.first)
    XCTAssertEqual(carPlay["UISceneClassName"], NSStringFromClass(CPTemplateApplicationScene.self))
    let className = try XCTUnwrap(carPlay["UISceneDelegateClassName"])
    XCTAssertTrue(NSClassFromString(className) === CarPlaySceneDelegate.self)
    XCTAssertTrue(class_conformsToProtocol(CarPlaySceneDelegate.self, CPTemplateApplicationSceneDelegate.self))
  }
}
