import CoreApi
import XCTest

final class DurationFormatterTests: XCTestCase {
  func testRoutingDurationsUseNearestMinute() {
    let minute = NSLocalizedString("minute", comment: "")
    let hour = NSLocalizedString("hour", comment: "")
    let language = Locale.current.languageCode ?? ""
    let unitSeparator = ["en", "de", "fr", "he", "fa", "ja", "ko", "mr", "th", "tr", "vi", "zh"].contains(language)
      ? "" : "\u{202F}"
    let groupingSeparator = ["ja", "zh"].contains(language) ? "" : "\u{00A0}"
    let cases: [(TimeInterval, String)] = [
      (0, "1\(unitSeparator)\(minute)"),
      (0.9, "1\(unitSeparator)\(minute)"),
      (59.999, "1\(unitSeparator)\(minute)"),
      (60, "1\(unitSeparator)\(minute)"),
      (89.999, "1\(unitSeparator)\(minute)"),
      (90, "2\(unitSeparator)\(minute)"),
      (269.5, "4\(unitSeparator)\(minute)"),
      (270, "5\(unitSeparator)\(minute)"),
      (1649.5, "27\(unitSeparator)\(minute)"),
      (1650, "28\(unitSeparator)\(minute)"),
      (1665, "28\(unitSeparator)\(minute)"),
      (2085, "35\(unitSeparator)\(minute)"),
      (3569.999, "59\(unitSeparator)\(minute)"),
      (3570, "1\(unitSeparator)\(hour)"),
      (3600, "1\(unitSeparator)\(hour)"),
      (3630, "1\(unitSeparator)\(hour)\(groupingSeparator)1\(unitSeparator)\(minute)"),
      (86400, "24\(unitSeparator)\(hour)"),
      (94485, "26\(unitSeparator)\(hour)\(groupingSeparator)15\(unitSeparator)\(minute)"),
      (2_147_483_647, "596523\(unitSeparator)\(hour)\(groupingSeparator)14\(unitSeparator)\(minute)"),
    ]
    for (seconds, expected) in cases {
      XCTAssertEqual(DurationFormatter.routingDurationString(from: seconds), expected, "\(seconds) seconds")
    }
  }
}
