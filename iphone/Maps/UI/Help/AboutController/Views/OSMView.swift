final class OSMView: UIView {

  private let OSMImageView = UIImageView()
  private let OSMTextLabel = UILabel()
  private var mapDate: String?

  var didTapHandler: (() -> Void)?

  init() {
    super.init(frame: .zero)
    setupViews()
    arrangeViews()
    layoutViews()
  }

  required init?(coder: NSCoder) {
    super.init(coder: coder)
    setupViews()
    arrangeViews()
    layoutViews()
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    guard let mapDate, traitCollection.userInterfaceStyle != previousTraitCollection?.userInterfaceStyle else { return }
    OSMTextLabel.attributedText = attributedString(for: mapDate)
  }

  // MARK: - Public
  func setMapDate(_ mapDate: String) {
    self.mapDate = mapDate
    OSMTextLabel.attributedText = attributedString(for: mapDate)
  }

  // MARK: - Private
  private func setupViews() {
    OSMImageView.image = UIImage(named: "osm_logo")

    OSMTextLabel.setFontStyle(.regular14, color: .blackPrimary)
    OSMTextLabel.lineBreakMode = .byWordWrapping
    OSMTextLabel.numberOfLines = 0
    OSMTextLabel.isUserInteractionEnabled = true
    
    let osmDidTapGesture = UITapGestureRecognizer(target: self, action: #selector(osmDidTap))
    OSMTextLabel.addGestureRecognizer(osmDidTapGesture)
  }

  private func arrangeViews() {
    addSubview(OSMImageView)
    addSubview(OSMTextLabel)
  }

  private func layoutViews() {
    OSMImageView.translatesAutoresizingMaskIntoConstraints = false
    OSMTextLabel.translatesAutoresizingMaskIntoConstraints = false

    NSLayoutConstraint.activate([
      OSMImageView.leadingAnchor.constraint(equalTo: leadingAnchor),
      OSMImageView.heightAnchor.constraint(equalToConstant: 40),
      OSMImageView.widthAnchor.constraint(equalTo: OSMImageView.heightAnchor),
      OSMImageView.topAnchor.constraint(greaterThanOrEqualTo: topAnchor),
      OSMImageView.bottomAnchor.constraint(lessThanOrEqualTo: bottomAnchor),

      OSMTextLabel.leadingAnchor.constraint(equalTo: OSMImageView.trailingAnchor, constant: 8),
      OSMTextLabel.trailingAnchor.constraint(equalTo: trailingAnchor),
      OSMTextLabel.topAnchor.constraint(greaterThanOrEqualTo: topAnchor),
      OSMTextLabel.bottomAnchor.constraint(lessThanOrEqualTo: bottomAnchor),
      OSMTextLabel.centerYAnchor.constraint(equalTo: OSMImageView.centerYAnchor)
    ])
  }

  @objc private func osmDidTap() {
    didTapHandler?()
  }

  private func attributedString(for date: String) -> NSAttributedString {
    let osmLink = "OpenStreetMap.org"
    let attributedString = NSMutableAttributedString(string: String(format: L("osm_presentation"), date.trimmingCharacters(in: .punctuationCharacters)),
                                                     attributes: [.font: UIFont.regular14(),
                                                                  .foregroundColor: StyleManager.shared.theme!.colors.blackPrimaryText]
    )
    let linkRange = attributedString.mutableString.range(of: osmLink)
    attributedString.addAttribute(.link, value: "https://www.openstreetmap.org/", range: linkRange)
    
    return attributedString
  }
}
