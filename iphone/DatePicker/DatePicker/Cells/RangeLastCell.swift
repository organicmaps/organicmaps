import UIKit

final class RangeLastCell: Cell {
  let rangeBgView = UIView()

  override func addSubviews() {
    contentView.addSubview(rangeBgView)
    rangeBgView.alignToSuperview(UIEdgeInsets(top: 4, left: 0, bottom: -4, right: -4))
    rangeBgView.layer.cornerRadius = 8
    rangeBgView.layer.cornerCurve = .continuous
    rangeBgView.layer.maskedCorners = [.layerMaxXMinYCorner, .layerMaxXMaxYCorner]
    super.addSubviews()
  }

  override func updateTheme() {
    super.updateTheme()
    label.textColor = theme.dayColor
    label.font = theme.dayFont
    rangeBgView.backgroundColor = theme.selectedRangeBackgroundColor
  }
}
