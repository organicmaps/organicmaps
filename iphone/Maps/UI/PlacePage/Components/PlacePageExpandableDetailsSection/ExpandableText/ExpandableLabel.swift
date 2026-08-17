final class ExpandableLabel: UIView {
  typealias TapHandler = () -> Void

  private enum Constants {
    static let defaultContentInsets = UIEdgeInsets(top: 8, left: 16, bottom: -8, right: -16)
  }

  private let stackView = UIStackView()
  private let textContainerView = UIView()
  private let expandView = ExpandView()
  private let tapGestureRecognizer = UITapGestureRecognizer()
  private var textContainer: any ExpandableTextContainer
  private var contentInsets: UIEdgeInsets = Constants.defaultContentInsets
  private var textContainerHeightConstraint: NSLayoutConstraint!
  private var contentViewHeightConstraint: NSLayoutConstraint?
  private var canExpand = false
  private var isExpanded = false
  private let didTap: TapHandler

  // MARK: - Public properties

  var text: String {
    didSet {
      guard text != oldValue else { return }
      updateText()
    }
  }

  // MARK: - Init

  init(expandableText: ExpandableText,
       textContainerFactory: any ExpandableTextContainerFactory.Type,
       contentInsets: UIEdgeInsets = Constants.defaultContentInsets,
       onExpand: @escaping TapHandler) {
    textContainer = textContainerFactory.makeContainer(for: expandableText)
    self.contentInsets = contentInsets
    didTap = onExpand
    text = expandableText.string
    super.init(frame: .zero)
    textContainer.onContentHeightChanged = { [weak self] in
      guard let self else { return }
      self.updateCollapsedState()
      self.superview?.layoutIfNeeded()
    }
    setupView()
    layoutView()
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    updateCollapsedState()
  }

  // MARK: - Private methods

  private func setupView() {
    clipsToBounds = true

    stackView.distribution = .fillProportionally
    stackView.axis = .vertical
    stackView.alignment = .fill
    stackView.clipsToBounds = true

    textContainerView.backgroundColor = .clear
    textContainerView.clipsToBounds = true

    expandView.isHidden = true
    expandView.setContentHuggingPriority(.defaultHigh, for: .vertical)
    expandView.setContentCompressionResistancePriority(.defaultHigh, for: .vertical)
    expandView.updateAppearance()

    tapGestureRecognizer.addTarget(self, action: #selector(onTap))
    tapGestureRecognizer.cancelsTouchesInView = false
    tapGestureRecognizer.delegate = self
    addGestureRecognizer(tapGestureRecognizer)
  }

  private func layoutView() {
    addSubview(stackView)
    stackView.addArrangedSubview(textContainerView)
    stackView.addArrangedSubview(expandView)
    stackView.alignToSuperview(contentInsets)

    textContainerHeightConstraint = textContainerView.heightAnchor.constraint(equalToConstant: 0)
    textContainerHeightConstraint.isActive = true

    installContentView()
  }

  private func installContentView() {
    textContainer.contentView.translatesAutoresizingMaskIntoConstraints = false
    textContainerView.addSubview(textContainer.contentView)
    let contentViewHeightConstraint = textContainer.contentView.heightAnchor.constraint(equalToConstant: 0)
    self.contentViewHeightConstraint = contentViewHeightConstraint
    NSLayoutConstraint.activate([
      textContainer.contentView.topAnchor.constraint(equalTo: textContainerView.topAnchor),
      textContainer.contentView.leadingAnchor.constraint(equalTo: textContainerView.leadingAnchor),
      textContainer.contentView.trailingAnchor.constraint(equalTo: textContainerView.trailingAnchor),
      contentViewHeightConstraint,
    ])
  }

  private func updateText() {
    textContainer.configure(with: text)
    setNeedsLayout()
  }

  private func updateCollapsedState() {
    let measuringWidth = bounds.width - contentInsets.left + contentInsets.right
    guard measuringWidth > 0 else { return }

    let fullHeight = textContainer.expandedHeight(for: measuringWidth)
    let collapsedHeight = textContainer.collapsedHeight(for: measuringWidth)
    canExpand = fullHeight > collapsedHeight
    let shouldCollapse = !isExpanded && canExpand

    contentViewHeightConstraint?.constant = fullHeight
    textContainerHeightConstraint.constant = shouldCollapse ? collapsedHeight : fullHeight
    expandView.isHidden = !shouldCollapse

    textContainer.contentView.isUserInteractionEnabled = isExpanded || !canExpand
  }

  // MARK: - Actions

  @objc
  private func onTap(_ sender: UITapGestureRecognizer) {
    guard sender.state == .ended else { return }
    didTap()
  }

  // MARK: - Public methods

  func setExpanded(_ expanded: Bool) {
    guard isExpanded != expanded else { return }

    guard bounds.width > 0 else {
      isExpanded = expanded
      setNeedsLayout()
      return
    }

    isExpanded = expanded
    if expanded {
      textContainer.updateContentHeight()
    }
    UIView.animate(withDuration: kFastAnimationDuration) {
      self.updateCollapsedState()
      self.superview?.layoutIfNeeded()
    }
  }
}

// MARK: - UIGestureRecognizerDelegate

extension ExpandableLabel: UIGestureRecognizerDelegate {
  func gestureRecognizer(_ gestureRecognizer: UIGestureRecognizer, shouldReceive _: UITouch) -> Bool {
    guard gestureRecognizer === tapGestureRecognizer else { return true }
    return canExpand && !isExpanded
  }

  func gestureRecognizer(_: UIGestureRecognizer,
                         shouldRecognizeSimultaneouslyWith _: UIGestureRecognizer) -> Bool {
    true
  }
}

private final class ExpandView: UIView {
  private enum Constants {
    static let gradientOverlapHeight: CGFloat = 28
  }

  let label = UILabel()
  private let gradientLayer = CAGradientLayer()

  override init(frame: CGRect) {
    super.init(frame: frame)
    setupView()
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    super.init(coder: coder)
    setupView()
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    gradientLayer.frame = CGRect(x: 0,
                                 y: -Constants.gradientOverlapHeight,
                                 width: bounds.width,
                                 height: bounds.height + Constants.gradientOverlapHeight)
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    updateAppearance()
  }

  func updateAppearance() {
    gradientLayer.startPoint = CGPoint(x: 0.5, y: 0)
    gradientLayer.endPoint = CGPoint(x: 0.5, y: 1)
    let color = UIColor.whitePrimary
    gradientLayer.colors = [
      color.withAlphaComponent(0).cgColor,
      color.cgColor,
    ]
    gradientLayer.locations = [0, 0.8]
  }

  private func setupView() {
    label.text = L("text_more_button")
    label.setFontStyle(.regular14, color: .linkBlue)

    backgroundColor = .clear
    clipsToBounds = false
    layer.addSublayer(gradientLayer)
    addSubview(label)

    label.translatesAutoresizingMaskIntoConstraints = false
    label.setContentHuggingPriority(.defaultHigh, for: .vertical)
    label.setContentCompressionResistancePriority(.defaultHigh, for: .vertical)
    NSLayoutConstraint.activate([
      label.leadingAnchor.constraint(equalTo: leadingAnchor),
      label.trailingAnchor.constraint(lessThanOrEqualTo: trailingAnchor),
      label.bottomAnchor.constraint(equalTo: bottomAnchor),
      label.topAnchor.constraint(greaterThanOrEqualTo: topAnchor),
    ])
  }
}
