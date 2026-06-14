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

  func testRespondsToForwardingSelectors() {
    let delegate = MainSceneDelegate()
    XCTAssertTrue(delegate.responds(to: #selector(MainSceneDelegate.scene(_:openURLContexts:))))
    XCTAssertTrue(delegate.responds(to: #selector(MainSceneDelegate.scene(_:continue:))))
    XCTAssertTrue(delegate.responds(to: #selector(MainSceneDelegate.windowScene(_:performActionFor:completionHandler:))))
  }
}
