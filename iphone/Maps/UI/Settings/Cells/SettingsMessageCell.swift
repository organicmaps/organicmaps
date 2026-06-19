final class SettingsMessageCell: MWMTableViewCell {
  private enum Constants {
    static let verticalInset: CGFloat = 6
  }

  private let messageLabel = UILabel(frame: .zero)

  override init(style _: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: .default, reuseIdentifier: reuseIdentifier)
    setupCell()
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    super.init(coder: coder)
  }

  override func prepareForReuse() {
    super.prepareForReuse()
    messageLabel.text = nil
  }

  func configure(text: String) {
    messageLabel.text = text
  }

  private func setupCell() {
    setStyle(.clearBackground)
    contentView.setStyle(.clearBackground)
    selectionStyle = .none
    isSeparatorHidden = true

    messageLabel.translatesAutoresizingMaskIntoConstraints = false
    messageLabel.numberOfLines = 0
    messageLabel.setFontStyleAndApply(.regular13, color: .red)

    contentView.addSubview(messageLabel)
    let margins = contentView.layoutMarginsGuide
    NSLayoutConstraint.activate([
      messageLabel.leadingAnchor.constraint(equalTo: margins.leadingAnchor),
      messageLabel.trailingAnchor.constraint(equalTo: margins.trailingAnchor),
      messageLabel.topAnchor.constraint(equalTo: contentView.topAnchor, constant: Constants.verticalInset),
      messageLabel.bottomAnchor.constraint(equalTo: contentView.bottomAnchor, constant: -Constants.verticalInset),
    ])
  }
}
