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

  func testRespondsToConnectAndDisconnectSelectors() {
    let delegate = CarPlaySceneDelegate()
    XCTAssertTrue(delegate.responds(to: #selector(CarPlaySceneDelegate.sceneDidBecomeActive(_:))))
    XCTAssertTrue(delegate.responds(to: #selector(CarPlaySceneDelegate.sceneWillResignActive(_:))))
    XCTAssertTrue(delegate.responds(to: #selector(CarPlaySceneDelegate.sceneWillEnterForeground(_:))))
    XCTAssertTrue(delegate.responds(to: #selector(CarPlaySceneDelegate.sceneDidEnterBackground(_:))))
    XCTAssertTrue(delegate.responds(to: #selector(CarPlaySceneDelegate.templateApplicationScene(_:didConnect:to:))))
    XCTAssertTrue(delegate.responds(to: #selector(CarPlaySceneDelegate.templateApplicationScene(_:didDisconnect:from:))))
  }
}
