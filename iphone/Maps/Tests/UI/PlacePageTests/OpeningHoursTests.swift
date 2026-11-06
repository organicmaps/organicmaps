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

  func test_GivenLunchBreak_WhenFormatted_ThenOpenShiftsAreListedOnSeparateLines() throws {
    let day = try today("Mo-Su 08:00-12:00,13:00-16:30")

    XCTAssertEqual(day.workingTimes, lines(time(8) + separator + "NOON", time(13) + separator + time(16, 30)))
  }

  func test_GivenTwoBreaks_WhenFormatted_ThenThreeShiftsAreListed() throws {
    let day = try today("Mo-Su 08:00-11:00,13:00-16:30,18:00-22:00")

    XCTAssertEqual(day.workingTimes, lines(time(8) + separator + time(11),
                                           time(13) + separator + time(16, 30),
                                           time(18) + separator + time(22)))
  }

  func test_GivenOvernightShiftAfterBreak_WhenFormatted_ThenItEndsNextMorning() throws {
    let day = try today("Mo-Su 11:45-14:00,18:00-02:00")

    XCTAssertEqual(day.workingTimes, lines(time(11, 45) + separator + time(14), time(18) + separator + time(2)))
  }

  func test_GivenWeekdayAndFridayShifts_WhenFormatted_ThenEachRowListsItsShifts() throws {
    // Issue #13927. Row 0 is today; the weekly rows follow in rule order.
    let days = try XCTUnwrap(OpeningHours(rawString: "Mo-Th 08:00-12:00, 13:00-16:30; Fr 08:00-12:00, 13:00-15:00",
                                          localization: localization)).days

    XCTAssertEqual(days.count, 4)
    XCTAssertEqual(days[1].workingTimes, lines(time(8) + separator + "NOON", time(13) + separator + time(16, 30)))
    XCTAssertEqual(days[2].workingTimes, lines(time(8) + separator + "NOON", time(13) + separator + time(15)))
  }

  private func today(_ rawString: String) throws -> WorkingDay {
    let openingHours = try XCTUnwrap(OpeningHours(rawString: rawString, localization: localization))
    return try XCTUnwrap(openingHours.days.first)
  }

  /// Formats a bound the way the place page does, so expectations hold in any test locale.
  private func time(_ hour: Int, _ minute: Int = 0) -> String {
    let date = Calendar.current.date(from: DateComponents(hour: hour, minute: minute))!
    return DateTimeFormatter.dateString(from: date, dateStyle: .none, timeStyle: .short)
  }

  private func lines(_ shifts: String...) -> String {
    shifts.joined(separator: "\n")
  }
}

private final class MockOpeningHoursLocalization: NSObject, IOpeningHoursLocalization {
  var closedString: String { "CLOSED" }
  var twentyFourSevenString: String { "24/7" }
  var allDayString: String { "ALLDAY" }
  var dailyString: String { "DAILY" }
  var todayString: String { "TODAY" }
  var dayOffString: String { "DAYOFF" }
  var noonString: String { "NOON" }
  var midnightString: String { "MIDNIGHT" }
}
