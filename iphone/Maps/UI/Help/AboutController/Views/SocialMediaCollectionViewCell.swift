final class SocialMediaCollectionViewCell: UICollectionViewCell {
  private let imageView = UIImageView()

  override init(frame: CGRect) {
    super.init(frame: frame)
    setupView()
  }

  required init?(coder: NSCoder) {
    super.init(coder: coder)
    setupView()
  }

  private func setupView() {
    setStyle(.clearBackground)

    imageView.contentMode = .scaleAspectFit
    imageView.translatesAutoresizingMaskIntoConstraints = false
    contentView.addSubview(imageView)

    NSLayoutConstraint.activate([
      imageView.topAnchor.constraint(equalTo: contentView.topAnchor),
      imageView.leadingAnchor.constraint(equalTo: contentView.leadingAnchor),
      imageView.trailingAnchor.constraint(equalTo: contentView.trailingAnchor),
      imageView.bottomAnchor.constraint(equalTo: contentView.bottomAnchor),
    ])
  }

  private func updateImageColor() {
    imageView.tintColor = .blackPrimaryText
  }

  // MARK: - Public

  func setImage(_ image: UIImage) {
    imageView.image = image
    updateImageColor()
  }
}
