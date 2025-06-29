final class TransportOptionCollectionViewCell: UICollectionViewCell {

  private let imageView = UIImageView()
  private(set) var routerType: MWMRouterType = .vehicle

  override var isSelected: Bool {
    didSet {
      setSelected(isSelected)
    }
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    imageView.layer.cornerRadius = imageView.bounds.width / 2
  }

  override init(frame: CGRect) {
    super.init(frame: frame)
    layout()
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  private func layout() {
    contentView.layer.setCornerRadius(.buttonDefault)
    imageView.translatesAutoresizingMaskIntoConstraints = false
    contentView.addSubview(imageView)
    NSLayoutConstraint.activate([
      imageView.topAnchor.constraint(equalTo: contentView.topAnchor),
      imageView.leadingAnchor.constraint(equalTo: contentView.leadingAnchor),
      imageView.trailingAnchor.constraint(equalTo: contentView.trailingAnchor),
      imageView.bottomAnchor.constraint(equalTo: contentView.bottomAnchor)
    ])
  }

  func configure(with routerType: MWMRouterType) {
    self.routerType = routerType
    self.isSelected = false
  }

  private func setSelected(_ selected: Bool) {
    imageView.image = routerType.image(for: selected)
    imageView.tintColor = selected ? .linkBlue() : .lightGray
  }
}
