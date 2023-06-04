import UIKit

public struct DatePickerViewTheme {
  public var monthHeaderBackgroundColor: UIColor = .white

  public var monthHeaderColor: UIColor = .darkText
  public var monthHeaderFont: UIFont = UIFont.systemFont(ofSize: 16, weight: .semibold)
  public var weekdaySymbolsColor: UIColor = .darkText
  public var weekdaySymbolsFont: UIFont = UIFont.systemFont(ofSize: 14, weight: .regular)

  public var dayColor: UIColor = .darkText
  public var dayFont: UIFont = UIFont.systemFont(ofSize: 16, weight: .regular)

  public var inactiveDayColor: UIColor = .lightGray
  public var inactiveDayFont: UIFont = UIFont.systemFont(ofSize: 16, weight: .regular)

  public var selectedDayColor: UIColor = .white
  public var selectedDayFont: UIFont = UIFont.systemFont(ofSize: 16, weight: .semibold)
  public var selectedDayBackgroundColor: UIColor = .systemBlue

  public var selectedRangeColor: UIColor = .white
  public var selectedRangeFont: UIFont = UIFont.systemFont(ofSize: 16, weight: .regular)
  public var selectedRangeBackgroundColor: UIColor = UIColor.systemBlue.withAlphaComponent(0.3)

  public init() {}
}
