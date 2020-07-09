class BookmarksSubscriptionButton: UIButton {
  private let descriptionLabel = UILabel()
  private let priceLabel = UILabel()

  override func awakeFromNib() {
    addSubview(descriptionLabel)
    addSubview(priceLabel)

    descriptionLabel.translatesAutoresizingMaskIntoConstraints = false
    priceLabel.translatesAutoresizingMaskIntoConstraints = false

    priceLabel.font = UIFont.semibold16()
    priceLabel.textAlignment = .right
    descriptionLabel.font = UIFont.semibold16()

    descriptionLabel.centerYAnchor.constraint(equalTo: centerYAnchor).isActive = true
    descriptionLabel.leadingAnchor.constraint(equalTo: leadingAnchor, constant: 16).isActive = true
    priceLabel.centerYAnchor.constraint(equalTo: centerYAnchor).isActive = true
    priceLabel.trailingAnchor.constraint(equalTo: trailingAnchor, constant: -16).isActive = true

    setTitle("", for: .normal)
  }

  override func setTitleColor(_ color: UIColor?, for state: UIControl.State) {
    super.setTitleColor(color, for: state)
    if state == .normal {
      descriptionLabel.textColor = color
      priceLabel.textColor = color
    }
  }

  override func setTitle(_ title: String?, for state: UIControl.State) {
    super.setTitle("", for: state)
  }

  func config(title: String, price: String, enabled: Bool) {
    descriptionLabel.text = title
    priceLabel.text = price
    isEnabled = enabled
  }
}
