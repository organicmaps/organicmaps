final class DonationView: UIView {

  private let donateTextLabel = UILabel()
  private let donateButton = UIButton()

  var donateButtonDidTapHandler: (() -> Void)?

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

  private func setupViews() {
    donateTextLabel.setFontStyle(.regular14, color: .blackPrimary)
    donateTextLabel.text = L("donate_description")
    donateTextLabel.textAlignment = .center
    donateTextLabel.lineBreakMode = .byWordWrapping
    donateTextLabel.numberOfLines = 0

    donateButton.setStyle(.flatNormalButton)
    donateButton.setTitle(L("donate").localizedUppercase, for: .normal)
    donateButton.addTarget(self, action: #selector(donateButtonDidTap), for: .touchUpInside)
  }

  private func arrangeViews() {
    addSubview(donateTextLabel)
    addSubview(donateButton)
  }

  private func layoutViews() {
    donateTextLabel.translatesAutoresizingMaskIntoConstraints = false
    donateButton.translatesAutoresizingMaskIntoConstraints = false

    NSLayoutConstraint.activate([
      donateTextLabel.leadingAnchor.constraint(equalTo: leadingAnchor),
      donateTextLabel.trailingAnchor.constraint(equalTo: trailingAnchor),
      donateTextLabel.topAnchor.constraint(equalTo: topAnchor),

      donateButton.topAnchor.constraint(equalTo: donateTextLabel.bottomAnchor, constant: 10),
      donateButton.widthAnchor.constraint(equalTo: widthAnchor, constant: -40).withPriority(.defaultHigh),
      donateButton.widthAnchor.constraint(lessThanOrEqualToConstant: 400).withPriority(.defaultHigh),
      donateButton.centerXAnchor.constraint(equalTo: centerXAnchor),
      donateButton.heightAnchor.constraint(equalToConstant: 40),
      donateButton.bottomAnchor.constraint(equalTo: bottomAnchor),
    ])
  }

  @objc private func donateButtonDidTap() {
    donateButtonDidTapHandler?()
  }
}
