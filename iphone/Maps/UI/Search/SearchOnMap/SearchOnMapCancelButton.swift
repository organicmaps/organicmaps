final class SearchOnMapCancelButton: UIControl {
  private enum Constants {
    static let contentInsets = NSDirectionalEdgeInsets(top: 0, leading: 6, bottom: 0, trailing: 16)
    static let iconInset: CGFloat = 8
    static let glassButtonSize: CGFloat = 44
  }

  private let button = UIButton()

  override init(frame: CGRect) {
    super.init(frame: frame)
    setupView()
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  private func setupView() {
    button.addTarget(self, action: #selector(buttonDidTap), for: .touchUpInside)
    addSubview(button)
    button.translatesAutoresizingMaskIntoConstraints = false

    if #available(iOS 26.0, *) {
      var configuration = UIButton.Configuration.glass()
      configuration.image = UIImage.icSearchClear
      configuration.baseForegroundColor = .label
      configuration.contentInsets = NSDirectionalEdgeInsets(top: Constants.iconInset,
                                                            leading: Constants.iconInset,
                                                            bottom: Constants.iconInset,
                                                            trailing: Constants.iconInset)
      button.configuration = configuration
      button.accessibilityLabel = L("cancel")
      NSLayoutConstraint.activate([
        button.centerYAnchor.constraint(equalTo: centerYAnchor),
        button.widthAnchor.constraint(equalToConstant: Constants.glassButtonSize),
        button.heightAnchor.constraint(equalToConstant: Constants.glassButtonSize),
      ])
    } else {
      button.setStyle(.searchCancelButton)
      button.setTitle(L("cancel"), for: .normal)
      NSLayoutConstraint.activate([
        button.topAnchor.constraint(equalTo: topAnchor, constant: Constants.contentInsets.top),
        button.bottomAnchor.constraint(equalTo: bottomAnchor, constant: -Constants.contentInsets.bottom),
      ])
    }

    NSLayoutConstraint.activate([
      button.leadingAnchor.constraint(equalTo: leadingAnchor, constant: Constants.contentInsets.leading),
      button.trailingAnchor.constraint(equalTo: trailingAnchor, constant: -Constants.contentInsets.trailing),
    ])
  }

  @objc private func buttonDidTap() {
    sendActions(for: .touchUpInside)
  }
}
