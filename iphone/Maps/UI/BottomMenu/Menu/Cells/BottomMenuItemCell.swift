import UIKit

class BottomMenuItemCell: UITableViewCell {

  private enum Constants {
    static let badgeSpacing: CGFloat = 8
    static let badgeBackgroundWidth: CGFloat = 32
  }

  @IBOutlet private var label: UILabel!
  @IBOutlet private var badgeBackground: UIView!
  @IBOutlet private var badgeCountLabel: UILabel!
  @IBOutlet private var separator: UIView!
  @IBOutlet private var icon: UIImageView!
  @IBOutlet private var badgeSpacingConstraint: NSLayoutConstraint!
  @IBOutlet private var badgeBackgroundWidthConstraint: NSLayoutConstraint!
  var anchorView: UIView {
    get {
      return icon
    }
  }

  private(set) var isEnabled: Bool = true

  func configure(image: UIImage,
                 title: String,
                 badgeCount: UInt = .zero,
                 imageStyle: GlobalStyleSheet = .black,
                 enabled: Bool = true) {
    icon.image = image
    label.text = title
    badgeBackground.isHidden = badgeCount == 0
    badgeCountLabel.text = "\(badgeCount)"
    if badgeCount == 0 {
      badgeSpacingConstraint.constant = 0
      badgeBackgroundWidthConstraint.constant = 0
    } else {
      badgeSpacingConstraint.constant = Constants.badgeSpacing
      badgeBackgroundWidthConstraint.constant = Constants.badgeBackgroundWidth
    }
    isEnabled = enabled
    icon.setStyleAndApply(isEnabled ? imageStyle : .gray)
    label.setFontStyleAndApply(isEnabled ? .blackPrimary : .blackHint)
  }
}
