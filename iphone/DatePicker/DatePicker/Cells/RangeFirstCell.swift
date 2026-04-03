import UIKit

final class RangeFirstCell: Cell {
  let rangeBgView = UIView()

  override func addSubviews() {
    contentView.addSubview(rangeBgView)
    rangeBgView.alignToSuperview(UIEdgeInsets(top: 4, left: 4, bottom: -4, right: 0))
    rangeBgView.layer.cornerRadius = 8
    rangeBgView.layer.cornerCurve = .continuous
    rangeBgView.layer.maskedCorners = [.layerMinXMinYCorner, .layerMinXMaxYCorner]
    super.addSubviews()
  }

  override func updateTheme() {
    super.updateTheme()
    label.textColor = theme.dayColor
    label.font = theme.dayFont
    rangeBgView.backgroundColor = theme.selectedRangeBackgroundColor
  }
}
