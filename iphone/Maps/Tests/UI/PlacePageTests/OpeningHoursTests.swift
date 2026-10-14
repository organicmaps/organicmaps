import CoreApi
import XCTest

final class OpeningHoursTests: XCTestCase {
  private let localization = MockOpeningHoursLocalization()
  /// The place page joins the bounds of a time span with thin spaces around the dash.
  private let separator = "\u{2009}-\u{2009}"

  func test_GivenSpanFromMidnightToNoon_WhenFormatted_ThenBothBoundsAreLabelled() throws {
    let day = try today("Mo-Su 00:00-12:00")

    XCTAssertEqual(day.workingTimes, "MIDNIGHT" + separator + "NOON")
  }

  func test_GivenSpanFromNoonToMidnight_WhenFormatted_ThenBothBoundsAreLabelled() throws {
    let day = try today("Mo-Su 12:00-24:00")

    XCTAssertEqual(day.workingTimes, "NOON" + separator + "MIDNIGHT")
  }

  func test_GivenHalfHourBounds_WhenFormatted_ThenTimesStayNumeric() throws {
    let day = try today("Mo-Su 00:30-12:30")

    XCTAssertFalse(day.workingTimes.contains("MIDNIGHT"))
    XCTAssertFalse(day.workingTimes.contains("NOON"))
  }

  func test_GivenSunEventBounds_WhenFormatted_ThenPlaceholderIsNotLabelled() throws {
    // Sun events carry no clock value here, only a 00:00 placeholder.
    let day = try today("Mo-Su sunrise-sunset")

    XCTAssertFalse(day.workingTimes.contains("MIDNIGHT"), day.workingTimes)
    XCTAssertFalse(day.workingTimes.contains("NOON"), day.workingTimes)
  }

  func test_GivenBreakStartingAtNoon_WhenFormatted_ThenBreakIsLabelled() throws {
    let day = try today("Mo-Su 08:00-12:00,13:00-20:00")

    let breaks = try XCTUnwrap(day.breaks)
    XCTAssertTrue(breaks.hasPrefix("BREAK NOON" + separator), breaks)
  }

  private func today(_ rawString: String) throws -> WorkingDay {
    let openingHours = try XCTUnwrap(OpeningHours(rawString: rawString, localization: localization))
    return try XCTUnwrap(openingHours.days.first)
  }
}

private final class MockOpeningHoursLocalization: NSObject, IOpeningHoursLocalization {
  var closedString: String { "CLOSED" }
  var breakString: String { "BREAK" }
  var twentyFourSevenString: String { "24/7" }
  var allDayString: String { "ALLDAY" }
  var dailyString: String { "DAILY" }
  var todayString: String { "TODAY" }
  var dayOffString: String { "DAYOFF" }
  var noonString: String { "NOON" }
  var midnightString: String { "MIDNIGHT" }
}
