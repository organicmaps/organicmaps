import CoreApi
import XCTest

final class DurationFormatterTests: XCTestCase {
  func testRoutingDurationsUseNearestMinute() {
    let minute = NSLocalizedString("minute", comment: "")
    let hour = NSLocalizedString("hour", comment: "")
    let space = "\u{00A0}"
    let cases: [(TimeInterval, String)] = [
      (0, "\u{2066}<1\u{2069}\(space)\(minute)"),
      (59.999, "\u{2066}<1\u{2069}\(space)\(minute)"),
      (60, "1\(space)\(minute)"),
      (269.5, "4\(space)\(minute)"),
      (270, "5\(space)\(minute)"),
      (1649.5, "27\(space)\(minute)"),
      (1650, "28\(space)\(minute)"),
      (1665, "28\(space)\(minute)"),
      (2085, "35\(space)\(minute)"),
      (3569.999, "59\(space)\(minute)"),
      (3570, "1\(space)\(hour)"),
      (3600, "1\(space)\(hour)"),
      (3630, "1\(space)\(hour)\(space)1\(space)\(minute)"),
      (86400, "24\(space)\(hour)"),
      (94485, "26\(space)\(hour)\(space)15\(space)\(minute)"),
    ]
    for (seconds, expected) in cases {
      XCTAssertEqual(DurationFormatter.routingDurationString(from: seconds), expected, "\(seconds) seconds")
    }
  }
}
