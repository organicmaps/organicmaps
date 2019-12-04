class ExpandableLabel: UIView {
  private var stackView = UIStackView()
  var textLabel = UILabel()
  var expandButton = UIButton()
  var expanded = false

  override init(frame: CGRect) {
    super.init(frame: frame)
    commonInit()
  }

  required init?(coder: NSCoder) {
    super.init(coder: coder)
    commonInit()
  }

  private func commonInit() {
    stackView.translatesAutoresizingMaskIntoConstraints = false
    stackView.axis = .vertical
    stackView.alignment = .leading
    textLabel.numberOfLines = 2
    textLabel.contentMode = .topLeft
    expandButton.clipsToBounds = true
    expandButton.addTarget(self, action: #selector(onExpand(_:)), for: .touchUpInside)
    addSubview(stackView)

    stackView.addArrangedSubview(textLabel)
    stackView.addArrangedSubview(expandButton)
    NSLayoutConstraint.activate([
      stackView.leftAnchor.constraint(equalTo: leftAnchor),
      stackView.topAnchor.constraint(equalTo: topAnchor),
      stackView.rightAnchor.constraint(equalTo: rightAnchor),
      stackView.bottomAnchor.constraint(equalTo: bottomAnchor)
    ])
  }

  @objc func onExpand(_ sender: UIButton) {
    UIView.animate(withDuration: kDefaultAnimationDuration) {
      self.textLabel.numberOfLines = 0
      self.expandButton.isHidden = true
      self.stackView.layoutIfNeeded()
    }
  }

  override func layoutSubviews() {
    super.layoutSubviews()

    guard let s = textLabel.text as NSString? else { return }
    let textRect = s.boundingRect(with: size,
                                  options: .usesLineFragmentOrigin,
                                  attributes: [.font: textLabel.font!],
                                  context: nil)
    let lineHeight = textLabel.font.lineHeight
    if Int(lineHeight * CGFloat(textLabel.numberOfLines)) >= Int(textRect.height) {
      expandButton.isHidden = true
    }
  }
}
