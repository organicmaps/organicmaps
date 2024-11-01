import Foundation

@objcMembers
public final class DateTimeFormatter: NSObject {
  private static let timeFormatter: DateComponentsFormatter = {
    let formatter = DateComponentsFormatter()
    formatter.allowedUnits = [.day, .hour, .minute]
    formatter.unitsStyle = .short
    formatter.maximumUnitCount = 2
    formatter.zeroFormattingBehavior = .dropAll
    return formatter
  }()
  private static let dateFormatter = DateFormatter()

  public static func durationString(from timeInterval: TimeInterval) -> String {
    timeFormatter.string(from: timeInterval) ?? ""
  }

  public static func dateString(from date: Date, dateStyle: DateFormatter.Style, timeStyle: DateFormatter.Style) -> String {
    DateFormatter.localizedString(from: date, dateStyle: dateStyle, timeStyle: timeStyle)
  }

  public static func dateString(from date: Date, locale: Locale = .current, format: String) -> String {
    dateFormatter.locale = locale
    dateFormatter.dateFormat = format
    return dateFormatter.string(from: date)
  }
}
