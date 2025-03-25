final class ElevationProfileView: UIView {

  private enum Constants {
    static let imageHeight: CGFloat = 60
  }

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

    imageView.backgroundColor = .clear
    imageView.contentMode = .scaleToFill
  }

  private func layout() {
    addSubview(imageView)

    imageView.translatesAutoresizingMaskIntoConstraints = false

    NSLayoutConstraint.activate([
      imageView.leadingAnchor.constraint(equalTo: leadingAnchor),
      imageView.trailingAnchor.constraint(equalTo: trailingAnchor),
      imageView.topAnchor.constraint(equalTo: topAnchor),
      imageView.bottomAnchor.constraint(equalTo: bottomAnchor),
      imageView.heightAnchor.constraint(equalToConstant: Constants.imageHeight)
    ])
  }

  func setImage(_ image: UIImage?) {
    imageView.image = image
    isHidden = image == nil ? true : false
  }
}
