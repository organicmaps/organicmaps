import UIKit

public protocol DatePickerViewDelegate: AnyObject {
  func datePickerView(_ view: DatePickerView, didSelect date: Date)
}

public final class DatePickerView: UIView {
  public weak var delegate: DatePickerViewDelegate?
  public var theme = DatePickerViewTheme() {
    didSet {
      collectionView.reloadData()
    }
  }

  var calendar = Calendar.autoupdatingCurrent
  let collectionView = UICollectionView(frame: .zero, collectionViewLayout: CalendarLayout())
  let cellStrategy: CellStrategy
  var year: Int
  var firstMonth = 1
  var numberOfMonths = 13

  public var minimumDate: Date {
    didSet {
      year = calendar.component(.year, from: minimumDate)
      firstMonth = calendar.component(.month, from: minimumDate)
      numberOfMonths = calendar.dateComponents([.month], from: minimumDate, to: maximumDate).month! + 1
      collectionView.reloadData()
    }
  }

  public var maximumDate: Date {
    didSet {
      numberOfMonths = calendar.dateComponents([.month], from: minimumDate, to: maximumDate).month! + 1
      collectionView.reloadData()
    }
  }

  public var startDate: Date? {
    didSet {
      endDate = nil
    }
  }

  public var endDate: Date? {
    didSet {
      if let endDate = endDate {
        guard let startDate = startDate, startDate < endDate else { fatalError("startDate must be less then endDate") }
      }
      collectionView.reloadData()
    }
  }

  override init(frame: CGRect) {
    minimumDate = Date()
    maximumDate = calendar.date(byAdding: .month, value: numberOfMonths - 1, to: minimumDate)!
    year = calendar.component(.year, from: minimumDate)
    firstMonth = calendar.component(.month, from: minimumDate)
    cellStrategy = CellStrategy(collectionView)
    super.init(frame: frame)
    config()
  }

  required init?(coder: NSCoder) {
    minimumDate = Date()
    maximumDate = calendar.date(byAdding: .month, value: numberOfMonths - 1, to: minimumDate)!
    year = calendar.component(.year, from: minimumDate)
    firstMonth = calendar.component(.month, from: minimumDate)
    cellStrategy = CellStrategy(collectionView)
    super.init(coder: coder)
    config()
  }

  private func config() {
    collectionView.register(CalendarHeader.self,
                            forSupplementaryViewOfKind: UICollectionView.elementKindSectionHeader,
                            withReuseIdentifier: "CalendarHeader")
    collectionView.backgroundColor = .clear
    addSubview(collectionView)
    collectionView.alignToSuperview()
    collectionView.dataSource = self
    collectionView.delegate = self
    collectionView.showsVerticalScrollIndicator = false
    collectionView.showsHorizontalScrollIndicator = false
  }

  private func dateAtIndexPath(_ indexPath: IndexPath) -> Date? {
    var components = DateComponents()
    components.year = year
    components.month = indexPath.section + firstMonth
    guard let month = calendar.date(from: components) else { return nil }
    let firstWeek = calendar.component(.weekOfMonth, from: month)
    components.weekday = indexPath.item % 7 + calendar.firstWeekday
    components.weekOfMonth = indexPath.item / 7 + firstWeek
    guard let date = calendar.date(from: components),
      calendar.isDate(date, equalTo: month, toGranularity: .month) else { return nil }
    return date
  }

  private func positionInRow(_ indexPath: IndexPath) -> PositionInRow {
    guard let date = dateAtIndexPath(indexPath) else { return .outside }
    var first = false
    var last = false

    let startOfMonthComponents = calendar.dateComponents([.year, .month], from: date)
    guard let startOfMonth = calendar.date(from: startOfMonthComponents) else { return .outside }

    if indexPath.item % 7 == 0 || calendar.isDate(date, equalTo: startOfMonth, toGranularity: .day) {
      first = true
    }

    var components = DateComponents()
    components.month = 1
    components.day = -1
    guard let endOfMonth = calendar.date(byAdding: components, to: startOfMonth) else {
      return first ? .first : .middle
    }

    if indexPath.item % 7 == 6 || calendar.isDate(date, equalTo: endOfMonth, toGranularity: .day){
      last = true
    }

    switch (first, last) {
    case (true, true):
      return .single
    case (true, _):
      return .first
    case (_, true):
      return .last
    default:
      return .middle
    }
  }

  private func isActiveDate(_ date: Date) -> Bool {
    return calendar.isDate(date, inSameDayAs: minimumDate) ||
      calendar.isDate(date, inSameDayAs: maximumDate) ||
      (date >= minimumDate && date <= maximumDate)
  }

  private func positionInRange(_ indexPath: IndexPath) -> PositionInRange {
    guard let date = dateAtIndexPath(indexPath) else { return .inactive }
    if !isActiveDate(date) { return .inactive }

    var state: PositionInRange = .outside
    guard let startDate = startDate else { return state }

    if calendar.isDate(date, inSameDayAs: startDate) {
      state = .first
    }

    guard let endDate = endDate else {
      return state == .first ? .single : state
    }

    if calendar.isDate(date, inSameDayAs: endDate) {
      state = .last
    } else if date > startDate && date < endDate {
      state = .middle
    }

    return state
  }

}

extension DatePickerView: UICollectionViewDataSource {
  public func numberOfSections(in collectionView: UICollectionView) -> Int {
    numberOfMonths
  }

  public func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
    var components = DateComponents()
    components.year = year
    components.month = section + firstMonth
    let date = calendar.date(from: components)!
    let range = calendar.range(of: .weekOfMonth, in: .month, for: date)!
    return range.count * 7
  }

  public func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
    let cell = cellStrategy.cell(positionInRange: positionInRange(indexPath),
                                 positionInRow: positionInRow(indexPath),
                                 indexPath: indexPath)
    cell.theme = theme
    guard let date = dateAtIndexPath(indexPath) else { return cell }
    let day = calendar.component(.day, from: date)
    cell.label.text = "\(day)"
    return cell
  }

  public func collectionView(_ collectionView: UICollectionView,
                             viewForSupplementaryElementOfKind kind: String,
                             at indexPath: IndexPath) -> UICollectionReusableView {
    switch kind {
    case UICollectionView.elementKindSectionHeader:
      let header = collectionView.dequeueReusableSupplementaryView(ofKind: kind,
                                                                   withReuseIdentifier: "CalendarHeader",
                                                                   for: indexPath) as! CalendarHeader
      header.theme = theme
      var components = DateComponents()
      components.year = year
      components.month = indexPath.section + firstMonth
      let date = calendar.date(from: components)
      let realComponents = calendar.dateComponents([.month, .year], from: date!)
      header.config("\(calendar.standaloneMonthSymbols[realComponents.month! - 1].capitalized) \(realComponents.year!)",
                    weekdays: calendar.shortStandaloneWeekdaySymbols.map { $0.capitalized },
                    firstWeekday: calendar.firstWeekday)
      return header
    default:
      fatalError()
    }
  }
}

extension DatePickerView: UICollectionViewDelegate {
  public func collectionView(_ collectionView: UICollectionView, shouldSelectItemAt indexPath: IndexPath) -> Bool {
    guard let date = dateAtIndexPath(indexPath) else { return false }
    return isActiveDate(date)
  }

  public func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    guard let date = dateAtIndexPath(indexPath) else { fatalError() }
    delegate?.datePickerView(self, didSelect: date)
  }
}
