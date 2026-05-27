import UIKit

class SelectedSingleCell: Cell {
  let selectedBgView = UIView()

  override func addSubviews() {
    contentView.addSubview(selectedBgView)
    selectedBgView.alignToSuperview(UIEdgeInsets(top: 4, left: 4, bottom: -4, right: -4))
    selectedBgView.layer.cornerRadius = 8
    selectedBgView.layer.cornerCurve = .continuous
    super.addSubviews()
  }

  override func updateTheme() {
    super.updateTheme()
    selectedBgView.backgroundColor = theme.selectedDayBackgroundColor
    label.textColor = theme.selectedDayColor
    label.font = theme.selectedDayFont
  }
}
