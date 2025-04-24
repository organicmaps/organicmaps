final class ElevationProfileView: UIView {

  private enum Constants {
    static let titleInsets = UIEdgeInsets(top: 4, left: 0, bottom: -8, right: 0)
    static let imageHeight: CGFloat = 60
    static let imageBottomOffset: CGFloat = -8
  }

  private let elevationTitleLabel = UILabel()
  private let imageView = UIImageView()

  init() {
    super.init(frame: .zero)
    setupView()
    layout()
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  private func setupView() {
    backgroundColor = .clear

    elevationTitleLabel.text = L("elevation_profile")
    elevationTitleLabel.setFontStyleAndApply(.regular14, color: .blackSecondary)

    imageView.backgroundColor = .clear
    imageView.contentMode = .scaleToFill
  }

  private func layout() {
    addSubview(elevationTitleLabel)
    addSubview(imageView)

    elevationTitleLabel.translatesAutoresizingMaskIntoConstraints = false
    imageView.translatesAutoresizingMaskIntoConstraints = false

    NSLayoutConstraint.activate([
      elevationTitleLabel.leadingAnchor.constraint(equalTo: leadingAnchor, constant: Constants.titleInsets.left),
      elevationTitleLabel.trailingAnchor.constraint(equalTo: trailingAnchor, constant: Constants.titleInsets.right),
      elevationTitleLabel.topAnchor.constraint(equalTo: topAnchor, constant: Constants.titleInsets.top),
      elevationTitleLabel.bottomAnchor.constraint(equalTo: imageView.topAnchor, constant: Constants.titleInsets.bottom),

      imageView.leadingAnchor.constraint(equalTo: leadingAnchor),
      imageView.trailingAnchor.constraint(equalTo: trailingAnchor),
      imageView.bottomAnchor.constraint(equalTo: bottomAnchor, constant: Constants.imageBottomOffset),
      imageView.heightAnchor.constraint(equalToConstant: Constants.imageHeight)
    ])
  }

  func setImage(_ image: UIImage?) {
    imageView.image = image
    isHidden = image == nil ? true : false
  }
}
