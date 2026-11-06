import Foundation

@objcMembers
class OpeinigHoursLocalization: NSObject, IOpeningHoursLocalization {
  var closedString: String {
    L("closed")
  }

  var twentyFourSevenString: String {
    L("twentyfour_seven")
  }

  var allDayString: String {
    L("editor_time_allday")
  }

  var noonString: String {
    L("noon")
  }

  var midnightString: String {
    L("midnight")
  }
}
