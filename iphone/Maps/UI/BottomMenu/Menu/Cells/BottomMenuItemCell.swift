final class BottomMenuItemCell: UITableViewCell {
  @IBOutlet private var label: UILabel!
  @IBOutlet private var badgeBackground: UIView!
  @IBOutlet private var badgeCountLabel: UILabel!
  @IBOutlet private var separator: UIView!
  @IBOutlet private var icon: UIImageView!
  var anchorView: UIView {
    icon
  }

  private(set) var isEnabled: Bool = true

  func configure(image: UIImage,
                 title: String,
                 badgeCount: UInt = .zero,
                 imageStyle: GlobalStyleSheet = .black,
                 enabled: Bool = true) {
    isEnabled = enabled
    icon.setStyleAndApply(enabled ? imageStyle : .gray)
    icon.image = image
    label.text = title
    label.setFontStyleAndApply(.regular16, color: enabled ? .blackPrimary : .blackHint)
    badgeBackground.isHidden = badgeCount == 0
    badgeCountLabel.text = "\(badgeCount)"
    badgeCountLabel.setFontStyleAndApply(.medium14, color: .whitePrimary)
    isAccessibilityElement = true
    accessibilityLabel = title
    accessibilityValue = badgeCount == 0 ? nil : String(format: L("menu_map_updates_count"), Int(badgeCount))
    accessibilityTraits = enabled ? [.button] : [.button, .notEnabled]
  }
}
