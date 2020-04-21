import UIKit

class BottomMenuItemCell: UITableViewCell {
  @IBOutlet private var label: UILabel!
  @IBOutlet private var badgeBackground: UIView!
  @IBOutlet private var badgeCountLabel: UILabel!
  @IBOutlet private var separator: UIView!
  @IBOutlet private var icon: UIImageView!
  var anchorView: UIView {
    get {
      return icon
    }
  }
  
  private var isEnabled: Bool = true
  private var isPromo: Bool = false
  
  func configure(imageName: String, title: String, badgeCount: UInt, enabled: Bool) {
    icon.image = UIImage.init(named: imageName)
    label.text = title
    badgeBackground.isHidden = badgeCount == 0
    badgeCountLabel.text = "\(badgeCount)"
    isEnabled = enabled
    icon.setStyleAndApply(isEnabled ? "MWMBlack" : "MWMGray")
    label.setStyleAndApply(isEnabled ? "blackPrimaryText" : "blackHintText")
  }
  
  func configure(imageName: String, title: String) {
    icon.image = UIImage.init(named: imageName)
    label.text = title
    icon.setStyleAndApply("MWMBlue")
    label.setStyleAndApply("linkBlueText")
    badgeBackground.isHidden = true
    isEnabled = true
    isPromo = true
  }
  
  override func setHighlighted(_ highlighted: Bool, animated: Bool) {
    guard isEnabled else {
      return
    }
    super.setHighlighted(highlighted, animated: animated)
    if isPromo {
      label.setStyleAndApply(highlighted ? "linkBlueHighlightedText" : "linkBlueText")
    } else {
      label.setStyleAndApply(highlighted ? "blackHintText" : "blackPrimaryText")
    }
  }
}
