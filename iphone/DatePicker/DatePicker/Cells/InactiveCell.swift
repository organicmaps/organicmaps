import UIKit

final class InactiveCell: Cell {
  override func updateTheme() {
    label.textColor = theme.inactiveDayColor
    label.font = theme.inactiveDayFont
  }
}
