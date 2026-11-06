import CoreApi
import XCTest

final class OpeningHoursTests: XCTestCase {
  /// NSCalendar weekday numbers.
  private enum Weekday: Int {
    case sunday = 1, monday, tuesday, wednesday, thursday, friday, saturday
  }

  private let localization = MockOpeningHoursLocalization()

  func test_GivenSpanFromMidnightToNoon_WhenFormatted_ThenBothBoundsAreLabelled() throws {
    let day = try today("Mo-Su 00:00-12:00")

    XCTAssertEqual(day.workingTimes, "MIDNIGHT—NOON")
  }

  func test_GivenSpanFromNoonToMidnight_WhenFormatted_ThenBothBoundsAreLabelled() throws {
    let day = try today("Mo-Su 12:00-24:00")

    XCTAssertEqual(day.workingTimes, "NOON—MIDNIGHT")
  }

  func test_GivenHalfHourBounds_WhenFormatted_ThenTimesStayNumeric() throws {
    let day = try today("Mo-Su 00:30-12:30")

    XCTAssertEqual(day.workingTimes, time(0, 30) + "—" + time(12, 30))
  }

  func test_GivenSunEventBounds_WhenFormatted_ThenPlaceholderIsNotLabelled() throws {
    // Sun events carry no clock value here, only a 00:00 placeholder.
    let day = try today("Mo-Su sunrise-sunset")

    XCTAssertFalse(day.workingTimes.contains("MIDNIGHT"), day.workingTimes)
    XCTAssertFalse(day.workingTimes.contains("NOON"), day.workingTimes)
  }

  func test_GivenLunchBreak_WhenFormatted_ThenOpenShiftsAreListedOnSeparateLines() throws {
    let day = try today("Mo-Su 08:00-12:00,13:00-16:30")

    XCTAssertEqual(day.workingTimes, lines(time(8) + "—NOON", time(13) + "—" + time(16, 30)))
  }

  func test_GivenTwoBreaks_WhenFormatted_ThenThreeShiftsAreListed() throws {
    let day = try today("Mo-Su 08:00-11:00,13:00-16:30,18:00-22:00")

    XCTAssertEqual(day.workingTimes, lines(time(8) + "—" + time(11),
                                           time(13) + "—" + time(16, 30),
                                           time(18) + "—" + time(22)))
  }

  func test_GivenOvernightShiftAfterBreak_WhenFormatted_ThenItEndsNextMorning() throws {
    let day = try today("Mo-Su 11:45-14:00,18:00-02:00")

    XCTAssertEqual(day.workingTimes, lines(time(11, 45) + "—" + time(14), time(18) + "—" + time(2)))
  }

  func test_GivenWeekdayAndFridayHours_WhenTodayIsThursday_ThenWeekStartsFromToday() throws {
    // Issue #13927.
    let days = try week("Mo-Th 08:00-12:00, 13:00-16:30; Fr 08:00-12:00, 13:00-15:00", today: .thursday)

    XCTAssertEqual(days.map(\.workingDays),
                   [name(.thursday), name(.friday), name(.saturday, .sunday), name(.monday, .wednesday)])
    XCTAssertEqual(days.map(\.workingTimes), [lunchBreak(until: 16, 30), lunchBreak(until: 15), "CLOSED",
                                              lunchBreak(until: 16, 30)])
    XCTAssertEqual(days.map(\.isOpen), [true, true, false, true])
  }

  func test_GivenClosedToday_WhenListed_ThenTodaysClosedDaysComeFirst() throws {
    let days = try week("Mo-Th 08:00-12:00, 13:00-16:30; Fr 08:00-12:00, 13:00-15:00", today: .saturday)

    XCTAssertEqual(days.map(\.workingDays), [name(.saturday, .sunday), name(.monday, .thursday), name(.friday)])
    XCTAssertEqual(days.map(\.workingTimes), ["CLOSED", lunchBreak(until: 16, 30), lunchBreak(until: 15)])
  }

  func test_GivenSameHoursEveryDay_WhenListed_ThenOneRowSpansTheWeekFromToday() throws {
    let days = try week("Mo-Su 10:00-20:00", today: .friday)

    XCTAssertEqual(days.map(\.workingDays), [name(.friday, .thursday)])
    XCTAssertEqual(days.map(\.workingTimes), [time(10) + "—" + time(20)])
  }

  func test_GivenAllDayOnWeekdays_WhenListed_ThenTheirRowSaysAllDay() throws {
    let days = try week("Mo-Fr 00:00-24:00; Sa 10:00-12:00,13:00-14:00", today: .monday)

    XCTAssertEqual(days.map(\.workingDays), [name(.monday, .friday), name(.saturday), name(.sunday)])
    XCTAssertEqual(days.map(\.workingTimes), ["ALLDAY", lines(time(10) + "—NOON", time(13) + "—" + time(14)),
                                              "CLOSED"])
  }

  func test_GivenOpenTwentyFourSeven_WhenListed_ThenOneRowSaysSo() throws {
    let days = try week("24/7", today: .monday)

    XCTAssertEqual(days.map(\.workingDays), ["TWENTYFOURSEVEN"])
    XCTAssertEqual(days.map(\.workingTimes), [""])
  }

  private func week(_ rawString: String, today: Weekday) throws -> [WorkingDay] {
    try XCTUnwrap(OpeningHours(rawString: rawString, localization: localization, today: today.rawValue)).days
  }

  private func today(_ rawString: String) throws -> WorkingDay {
    try XCTUnwrap(week(rawString, today: .monday).first)
  }

  /// Formats a bound the way the place page does, so expectations hold in any test locale.
  private func time(_ hour: Int, _ minute: Int = 0) -> String {
    let date = Calendar.current.date(from: DateComponents(hour: hour, minute: minute))!
    return DateTimeFormatter.dateString(from: date, dateStyle: .none, timeStyle: .short)
  }

  private func name(_ first: Weekday, _ last: Weekday? = nil) -> String {
    let names = Calendar.current.shortStandaloneWeekdaySymbols
    guard let last else { return names[first.rawValue - 1] }
    return names[first.rawValue - 1] + "-" + names[last.rawValue - 1]
  }

  private func lunchBreak(until hour: Int, _ minute: Int = 0) -> String {
    lines(time(8) + "—NOON", time(13) + "—" + time(hour, minute))
  }

  private func lines(_ shifts: String...) -> String {
    shifts.joined(separator: "\n")
  }
}

private final class MockOpeningHoursLocalization: NSObject, IOpeningHoursLocalization {
  var closedString: String { "CLOSED" }
  var twentyFourSevenString: String { "TWENTYFOURSEVEN" }
  var allDayString: String { "ALLDAY" }
  var noonString: String { "NOON" }
  var midnightString: String { "MIDNIGHT" }
}
