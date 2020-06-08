final class BottomMenuLayerButton: VerticallyAlignedButton {
  private var badgeView: UIView?
  private let badgeSize = CGSize(width: 12, height: 12)
  private let badgeOffset = CGPoint(x: -3, y: 3)

  var isBadgeHidden: Bool = true{
    didSet {
      if oldValue != isBadgeHidden {
        updateBadge()
      }
    }
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    updateBadge()
  }

  private func updateBadge() {
    if isBadgeHidden {
      badgeView?.removeFromSuperview()
      badgeView = nil
    } else {
      if badgeView == nil {
        badgeView = UIView()
        badgeView?.styleName = "Badge"
        addSubview(badgeView!)
      }
      let imageFrame = imageView.frame
      badgeView?.frame = CGRect(x:imageFrame.minX + imageFrame.width - badgeSize.width / 2 + badgeOffset.x,
                                y:imageFrame.minY - badgeSize.height/2 + badgeOffset.y,
                                width: badgeSize.width,
                                height: badgeSize.height)
    }
  }
}
