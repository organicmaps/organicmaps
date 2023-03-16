final class TransportTransitSeparator: UICollectionReusableView {
  override init(frame: CGRect) {
    super.init(frame: frame)
    setup()
  }

  required init?(coder aDecoder: NSCoder) {
    super.init(coder: aDecoder)
    setup()
  }

  private func setup() {
    let image = UIImageView(image: #imageLiteral(resourceName: "ic_arrow"))
    image.styleName = "MWMBlack"
    image.contentMode = .scaleAspectFit
    addSubview(image)
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    subviews.first!.frame = bounds
  }
}
