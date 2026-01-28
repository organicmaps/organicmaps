import UIKit

final class CalendarHeader: UICollectionReusableView {
  let monthLabel = UILabel()
  let vStack = UIStackView()
  let weekdaysStack = UIStackView()
  var weekdayLabels: [UILabel] = []
  var theme = DatePickerViewTheme() {
    didSet {
      updateTheme()
    }
  }

  private func updateTheme() {
    backgroundColor = theme.monthHeaderBackgroundColor
    monthLabel.textColor = theme.monthHeaderColor
    monthLabel.font = theme.monthHeaderFont
    weekdayLabels.forEach {
      $0.textColor = theme.weekdaySymbolsColor
      $0.font = theme.weekdaySymbolsFont
    }
  }

  override init(frame: CGRect) {
    super.init(frame: frame)
    addSubview(vStack)
    vStack.alignToSuperview()
    vStack.addArrangedSubview(monthLabel)
    vStack.addArrangedSubview(weekdaysStack)
    for _ in 0..<7 {
      let label = UILabel()
      label.textAlignment = .center
      weekdayLabels.append(label)
      weekdaysStack.addArrangedSubview(label)
    }
    vStack.axis = .vertical
    vStack.distribution = .fillEqually
    weekdaysStack.axis = .horizontal
    weekdaysStack.distribution = .fillEqually
    monthLabel.textAlignment = .center
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func prepareForReuse() {
    monthLabel.text = nil
  }

  func config(_ month: String, weekdays: [String], firstWeekday: Int) {
    monthLabel.text = month
    for i in 0..<7 {
      let index = (i + firstWeekday - 1) % 7
      let label = weekdayLabels[i]
      label.text = weekdays[index]
    }
  }
}
