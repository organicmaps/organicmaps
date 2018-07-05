@IBDesignable
class VerticallyAlignedButton: MWMButton {

  override init(frame: CGRect) {
    super.init(frame: frame)
    titleLabel?.textAlignment = .center
  }

  required init?(coder aDecoder: NSCoder) {
    super.init(coder: aDecoder)
    titleLabel?.textAlignment = .center
  }

  override func contentRect(forBounds bounds: CGRect) -> CGRect {
    let w = CGFloat.maximum(intrinsicContentSize.width, bounds.width)
    let h = CGFloat.maximum(intrinsicContentSize.height, bounds.height)
    return CGRect(x: bounds.minX, y: bounds.minY, width: w, height: h)
  }

  override func titleRect(forContentRect contentRect: CGRect) -> CGRect {
    let titleRect = super.titleRect(forContentRect: contentRect)
    return CGRect(x: contentRect.minX,
                  y: contentRect.height - titleRect.height,
                  width: contentRect.width,
                  height: titleRect.height)
  }

  override func imageRect(forContentRect contentRect: CGRect) -> CGRect {
    let imageRect = super.imageRect(forContentRect: contentRect)
    return CGRect(x: floor((contentRect.width - imageRect.width) / 2),
                  y: 0,
                  width: imageRect.width,
                  height: imageRect.height)
  }

  override var intrinsicContentSize: CGSize {
    let r = CGRect(x: 0, y: 0, width: CGFloat.greatestFiniteMagnitude, height: CGFloat.greatestFiniteMagnitude)
    let imageRect = super.imageRect(forContentRect: r)
    let titleRect = super.titleRect(forContentRect: r)
    let w = CGFloat.maximum(imageRect.width, titleRect.width)

    return CGSize(width: w, height: imageRect.height + titleRect.height + 4)
  }
}
