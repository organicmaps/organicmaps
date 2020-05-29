class BottomMenuLayerButton: VerticallyAlignedButton {
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

  private func updateBadge() {
    if isBadgeHidden {
      badgeView?.removeFromSuperview()
    } else if let imageView = imageView{
      badgeView = UIView()
      badgeView?.styleName = "Badge"
      let imageFrame = imageView.frame
      badgeView?.frame = CGRect(x:imageFrame.minX + imageFrame.width - badgeSize.width / 2 + badgeOffset.x,
                                y:imageFrame.minY - badgeSize.height/2 + badgeOffset.y,
                                width: badgeSize.width,
                                height: badgeSize.height)
      addSubview(badgeView!)
    }
  }
}
