@IBDesignable
class LeftAlignedIconButton: UIButton {
  override func layoutSubviews() {
    super.layoutSubviews()
    contentHorizontalAlignment = .left
    let availableSpace = UIEdgeInsetsInsetRect(bounds, contentEdgeInsets)
    let imageWidth = imageView?.frame.width ?? 0
    let titleWidth = titleLabel?.frame.width ?? 0
    let availableWidth = availableSpace.width - imageEdgeInsets.right - imageWidth * 2 - titleWidth
    titleEdgeInsets = UIEdgeInsets(top: 0, left: floor(availableWidth) / 2, bottom: 0, right: 0)
  }
}

