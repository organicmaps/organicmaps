@testable import Organic_Maps__Debug_
import XCTest

final class ExternalGPSManagerTests: XCTestCase {
  // One GPS week-number rollover period: 1024 weeks.
  private let rolloverPeriod: TimeInterval = 1024 * 7 * 24 * 60 * 60

  // A fixed "now" in Nov 2026 so the tests are deterministic.
  private let now = Date(timeIntervalSince1970: 1_793_000_000)

  func test_GivenFixFromThisMoment_WhenCorrecting_ThenUnchanged() {
    let fix = now.addingTimeInterval(-2)  // a healthy fix is seconds old
    let corrected = ExternalGPSManager.correctWeekRollover(fix, now: now)
    XCTAssertEqual(corrected.timeIntervalSince1970, fix.timeIntervalSince1970, accuracy: 0.001)
  }

  func test_GivenFixWithinWeekSlack_WhenCorrecting_ThenUnchanged() {
    // Six days old is still within the one-week slack, so it must not be "corrected".
    let fix = now.addingTimeInterval(-6 * 24 * 60 * 60)
    let corrected = ExternalGPSManager.correctWeekRollover(fix, now: now)
    XCTAssertEqual(corrected.timeIntervalSince1970, fix.timeIntervalSince1970, accuracy: 0.001)
  }

  func test_GivenSingleRollover_WhenCorrecting_ThenAdvancedToTrueTime() {
    // The XGPS160 reports a date exactly one rollover period in the past.
    let trueFix = now.addingTimeInterval(-3)
    let reported = trueFix.addingTimeInterval(-rolloverPeriod)
    let corrected = ExternalGPSManager.correctWeekRollover(reported, now: now)
    XCTAssertEqual(corrected.timeIntervalSince1970, trueFix.timeIntervalSince1970, accuracy: 0.001)
  }

  func test_GivenMultipleRollovers_WhenCorrecting_ThenAdvancedToTrueTime() {
    // A date two rollover periods back must take two corrections.
    let trueFix = now.addingTimeInterval(-10)
    let reported = trueFix.addingTimeInterval(-2 * rolloverPeriod)
    let corrected = ExternalGPSManager.correctWeekRollover(reported, now: now)
    XCTAssertEqual(corrected.timeIntervalSince1970, trueFix.timeIntervalSince1970, accuracy: 0.001)
  }

  func test_GivenRealXGPS160Date_WhenCorrecting_ThenBecomesCurrent() {
    // The actual on-device capture (XGPS160 firmware 2.5.4): RMC date 170307 → 2007-03-17 16:01:44 UTC.
    var cal = Calendar(identifier: .gregorian)
    cal.timeZone = TimeZone(identifier: "UTC")!
    let reported = cal.date(from: DateComponents(
      year: 2007, month: 3, day: 17, hour: 16, minute: 1, second: 44))!
    let corrected = ExternalGPSManager.correctWeekRollover(reported, now: now)

    // Advanced by a whole number of rollover periods and now within a week of "now".
    let advanced = corrected.timeIntervalSince(reported)
    XCTAssertEqual(advanced.truncatingRemainder(dividingBy: rolloverPeriod), 0, accuracy: 0.001)
    XCTAssertGreaterThan(corrected, now.addingTimeInterval(-7 * 24 * 60 * 60))
  }
}
