import Foundation
extension MWMTableViewCell {
  @objc override func applyTheme() {
    if styleName.isEmpty {
      styleName = "MWMTableViewCell"
    }
    for style in StyleManager.shared.getStyle(styleName)
      where !style.isEmpty && !style.hasExclusion(view: self) {
        MWMTableViewCellRenderer.render(self, style: style)
    }
  }
}

class MWMTableViewCellRenderer: UITableViewCellRenderer {
  class func render(_ control: MWMTableViewCell, style: Style) {
    super.render(control, style: style)
  }
}
