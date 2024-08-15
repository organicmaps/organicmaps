import UIKit

class BottomMenuItemCell: UITableViewCell {
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

  func configure(imageName: String, title: String, badgeCount: UInt = .zero, enabled: Bool = true) {
    icon.image = UIImage(named: imageName)
    label.text = title
    badgeBackground.isHidden = badgeCount == 0
    badgeCountLabel.text = "\(badgeCount)"
    if badgeCount == 0 {
      badgeSpacingConstraint.constant = 0
      badgeBackgroundWidthConstraint.constant = 0
    } else {
      badgeSpacingConstraint.constant = 8
      badgeBackgroundWidthConstraint.constant = 32
    }
    isEnabled = enabled
    icon.setStyleAndApply(isEnabled ? "MWMBlack" : "MWMGray")
    label.setStyleAndApply(isEnabled ? "blackPrimaryText" : "blackHintText")
  }

  override func setHighlighted(_ highlighted: Bool, animated: Bool) {
    guard isEnabled else {
      return
    }
    super.setHighlighted(highlighted, animated: animated)
    label.setStyleAndApply(highlighted ? "blackHintText" : "blackPrimaryText")
  }

  func setState(_ state: TrackRecordingState) {
    switch state {
    case .inactive, .error:
      // TODO: localize
      configure(imageName: "track_recorder_inactive", title: L("Record Track"))
    case .active:
      // TODO: localize
      configure(imageName: "track_recorder_active", title: L("Stop Track Recording"))
    }
  }
}
