final class ReviewView: UIView {
  private let stackView = UIStackView()
  let authorLabel = UILabel()
  let dateLabel = UILabel()
  let reviewLabel = ExpandableLabel()
  let separator = UIView()

  override init(frame: CGRect) {
    super.init(frame: frame)
    commonInit()
  }

  required init?(coder: NSCoder) {
    super.init(coder: coder)
    commonInit()
  }

  private func commonInit() {
    stackView.axis = .vertical
    stackView.alignment = .leading
    stackView.spacing = 4
    addSubview(stackView)

    stackView.addArrangedSubview(authorLabel)
    stackView.addArrangedSubview(dateLabel)
    stackView.addArrangedSubview(reviewLabel)
    stackView.setCustomSpacing(8, after: dateLabel)
    stackView.alignToSuperview(UIEdgeInsets(top: 16, left: 16, bottom: -8, right: -16))

    separator.translatesAutoresizingMaskIntoConstraints = false
    addSubview(separator)
    NSLayoutConstraint.activate([
      separator.heightAnchor.constraint(equalToConstant: 1),
      separator.leftAnchor.constraint(equalTo: authorLabel.leftAnchor),
      separator.bottomAnchor.constraint(equalTo: bottomAnchor),
      separator.rightAnchor.constraint(equalTo: rightAnchor)
    ])
  }
}

extension ReviewView {
  func defaultConfig() {
    authorLabel.font = UIFont.bold14()
    authorLabel.textColor = UIColor.blackPrimaryText()
    dateLabel.font = UIFont.regular12()
    dateLabel.textColor = UIColor.blackSecondaryText()
    reviewLabel.font = UIFont.regular14()
    reviewLabel.textColor = UIColor.blackPrimaryText()
    reviewLabel.expandColor = UIColor.linkBlue()
    reviewLabel.expandText = L("placepage_more_button")
    separator.backgroundColor = UIColor.blackDividers()
  }
}
