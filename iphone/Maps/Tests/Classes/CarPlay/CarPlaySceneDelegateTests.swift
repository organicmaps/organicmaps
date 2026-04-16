import CarPlay
@testable import Organic_Maps__Debug_
import XCTest

final class CarPlaySceneDelegateTests: XCTestCase {
  /// OMaps.plist references "CarPlaySceneDelegate" as UISceneDelegateClassName. The Swift class
  /// needs a stable @objc name for the Obj-C runtime to resolve it, because the Debug product's
  /// Swift module is mangled to Organic_Maps__Debug_.
  func testRuntimeClassNameMatchesPlist() {
    XCTAssertEqual(NSStringFromClass(CarPlaySceneDelegate.self), "CarPlaySceneDelegate")
  }

  func testConformsToCPTemplateApplicationSceneDelegate() {
    XCTAssertTrue(CarPlaySceneDelegate() is CPTemplateApplicationSceneDelegate)
  }

  func testRespondsToConnectAndDisconnectSelectors() {
    let delegate = CarPlaySceneDelegate()
    XCTAssertTrue(delegate.responds(to: Selector(("templateApplicationScene:didConnectInterfaceController:toWindow:"))))
    XCTAssertTrue(delegate.responds(to: Selector(("templateApplicationScene:didDisconnectInterfaceController:fromWindow:"))))
  }
}
