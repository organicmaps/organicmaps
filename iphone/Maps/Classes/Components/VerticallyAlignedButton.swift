@IBDesignable
class VerticallyAlignedButton: MWMButton {
  @IBInspectable
  var spacing: CGFloat = 4
  @IBInspectable
  var numberOfLines: Int {
    get {
      return titleLabel?.numberOfLines ?? 0
    }
    set {
      titleLabel?.numberOfLines = newValue
    }
  }

  override init(frame: CGRect) {
    super.init(frame: frame)
    titleLabel?.textAlignment = .center
  }

  required init?(coder aDecoder: NSCoder) {
    super.init(coder: aDecoder)
    titleLabel?.textAlignment = .center
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    updateView()
  }

  private func updateView() {
    let imageSize = self.imageView?.frame.size ?? .zero
    let titleSize = self.titleLabel?.frame.size ?? .zero
    let size = self.size
    let height = imageSize.height + titleSize.height + spacing
    self.imageEdgeInsets = UIEdgeInsets(top: -(size.height - imageSize.height),
                                        left: 0,
                                        bottom: 0,
                                        right: -titleSize.width)
    self.titleEdgeInsets = UIEdgeInsets(top: height,
                                        left: -imageSize.width,
                                        bottom: 0,
                                        right: 0)
  }
}
