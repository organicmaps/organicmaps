import DatePicker

extension DatePickerView {
  override func applyTheme() {
    if styleName.isEmpty {
      styleName = "DatePickerView"
    }
    for style in StyleManager.shared.getStyle(styleName) where !style.isEmpty && !style.hasExclusion(view: self) {
      DatePickerViewRenderer.render(self, style: style)
    }
  }
}

fileprivate final class DatePickerViewRenderer {
  class func render(_ control: DatePickerView, style: Style) {
    control.backgroundColor = style.backgroundColor

    var theme = DatePickerViewTheme()
    theme.monthHeaderBackgroundColor = style.backgroundColor!
    theme.monthHeaderColor = style.fontColorDisabled!
    theme.weekdaySymbolsColor = style.fontColorDisabled!
    theme.dayColor = style.fontColor!
    theme.selectedDayColor = style.fontColorSelected!
    theme.selectedDayBackgroundColor = style.backgroundColorSelected!
    theme.selectedRangeBackgroundColor = style.backgroundColorHighlighted!
    theme.inactiveDayColor = style.fontColorDisabled!
    control.theme = theme
  }
}
