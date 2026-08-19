@testable import Organic_Maps__Debug_
import UIKit
import XCTest

final class ThemeManagerTests: XCTestCase {
  func testResolveMapTheme() {
    let testCases: [(String, MWMTheme, Bool, UIUserInterfaceStyle, MWMTheme)] = [
      ("day", .day, false, .light, .day),
      ("vehicle day", .day, true, .light, .vehicleDay),
      ("night", .night, false, .light, .night),
      ("vehicle night", .night, true, .light, .vehicleNight),
      ("auto light", .auto, false, .light, .day),
      ("auto vehicle light", .auto, true, .light, .vehicleDay),
      ("auto dark", .auto, false, .dark, .night),
      ("auto vehicle dark", .auto, true, .dark, .vehicleNight),
      ("stale vehicle day", .vehicleDay, false, .light, .day),
      ("stale vehicle night", .vehicleNight, false, .dark, .night),
    ]

    for (name, preference, isVehicleNavigation, systemStyle, expected) in testCases {
      XCTAssertEqual(
        ThemeManager.resolveMapTheme(
          preference,
          isVehicleNavigation: isVehicleNavigation,
          systemStyle: systemStyle
        ),
        expected,
        name
      )
    }
  }
}
