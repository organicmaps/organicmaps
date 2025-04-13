import Foundation

class CarplayPlaceholderView: UIView {
  private let containerView = UIView()
  private let imageView = UIImageView()
  private let descriptionLabel = UILabel()
  private let switchButton = UIButton(type: .system);

  override init(frame: CGRect) {
    super.init(frame: frame)
    commonInit()
  }

  required init?(coder: NSCoder) {
    super.init(coder: coder)
    commonInit()
  }

  private func commonInit() {
    addSubview(containerView)

    imageView.image = UIImage(named: "ic_carplay_activated")
    imageView.contentMode = .scaleAspectFit
    containerView.addSubview(imageView)

    descriptionLabel.text = L("car_used_on_the_car_screen")
    descriptionLabel.font = UIFont.bold24()
    descriptionLabel.textAlignment = .center
    descriptionLabel.numberOfLines = 0
    containerView.addSubview(descriptionLabel)

    switchButton.setTitle(L("car_continue_on_the_phone"), for: .normal)
    switchButton.addTarget(self, action: #selector(onSwitchButtonTap), for: .touchUpInside)
    switchButton.titleLabel?.font = UIFont.medium16()
    switchButton.titleLabel?.lineBreakMode = .byWordWrapping
    switchButton.titleLabel?.textAlignment = .center
    switchButton.layer.cornerRadius = 8
    containerView.addSubview(switchButton)

    updateColors()
    setupConstraints()
  }

  override func applyTheme() {
    super.applyTheme()
    updateColors()
  }

  private func updateColors() {
    backgroundColor = UIColor.carplayPlaceholderBackground()
    descriptionLabel.textColor = UIColor.blackSecondaryText()
    switchButton.backgroundColor = UIColor.linkBlue()
    switchButton.setTitleColor(UIColor.whitePrimaryText(), for: .normal)
  }

  @objc private func onSwitchButtonTap(_ sender: UIButton) {
    CarPlayService.shared.showOnPhone()
  }

  private func setupConstraints() {
    translatesAutoresizingMaskIntoConstraints = false

    containerView.translatesAutoresizingMaskIntoConstraints = false
    imageView.translatesAutoresizingMaskIntoConstraints = false
    descriptionLabel.translatesAutoresizingMaskIntoConstraints = false
    switchButton.translatesAutoresizingMaskIntoConstraints = false

    let horizontalPadding: CGFloat = 24

    NSLayoutConstraint.activate([
      containerView.centerYAnchor.constraint(equalTo: centerYAnchor),
      containerView.topAnchor.constraint(greaterThanOrEqualTo: topAnchor),
      containerView.leadingAnchor.constraint(equalTo: leadingAnchor),
      containerView.trailingAnchor.constraint(equalTo: trailingAnchor),
      containerView.bottomAnchor.constraint(lessThanOrEqualTo: bottomAnchor),

      imageView.topAnchor.constraint(equalTo: containerView.topAnchor),
      imageView.centerXAnchor.constraint(equalTo: centerXAnchor),
      imageView.widthAnchor.constraint(equalToConstant: 160),
      imageView.heightAnchor.constraint(equalToConstant: 160),

      descriptionLabel.topAnchor.constraint(equalTo: imageView.bottomAnchor, constant: 32),
      descriptionLabel.leadingAnchor.constraint(equalTo: leadingAnchor, constant: horizontalPadding),
      descriptionLabel.trailingAnchor.constraint(equalTo: trailingAnchor, constant: -horizontalPadding),

      switchButton.topAnchor.constraint(equalTo: descriptionLabel.bottomAnchor, constant: 24),
      switchButton.leadingAnchor.constraint(equalTo: leadingAnchor, constant: horizontalPadding),
      switchButton.trailingAnchor.constraint(equalTo: trailingAnchor, constant: -horizontalPadding),
      switchButton.heightAnchor.constraint(equalToConstant: 48),
      switchButton.bottomAnchor.constraint(equalTo: containerView.bottomAnchor)
    ])
  }
}
