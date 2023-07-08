import UIKit

final class RangeMiddleCell: Cell {
  let rangeBgView = UIView()

  override func addSubviews() {
    contentView.addSubview(rangeBgView)
    rangeBgView.alignToSuperview(UIEdgeInsets(top: 4, left: 0, bottom: -4, right: 0))
    super.addSubviews()
  }

  override func updateTheme() {
    super.updateTheme()
    label.textColor = theme.dayColor
    label.font = theme.dayFont
    rangeBgView.backgroundColor = theme.selectedRangeBackgroundColor
  }
}
