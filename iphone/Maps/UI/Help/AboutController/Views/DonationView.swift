final class DonationView: UIButton {
  private enum Constants {
    static let contentInsets = UIEdgeInsets(top: 12, left: 16, bottom: -16, right: -16)
    static let backgroundColorHex = "094600"
    static let ribbonSize = CGSize(width: 75, height: 75)
    static let actionButtonSpacing = CGFloat(10)
    static let actionButtonHeight = CGFloat(40)
    static let actionButtonMinWidth = CGFloat(250)
    static let actionButtonMaxWidth = CGFloat(450)
    static let actionButtonTitleInsets = UIEdgeInsets(top: 4, left: 12, bottom: 4, right: 12)
    static let gradientColorMultiplier: CGFloat = 0.3
    static let pressScale: CGFloat = 0.97
    static let tapAnimationDuration: TimeInterval = kFastAnimationDuration
    static let highlightAlpha: CGFloat = 0.12
  }

  private let descriptionLabel = UILabel()
  private let actionLabelBackgroundView = UIView()
  private let actionLabel = UILabel()
  private let backgroundImageView = UIImageView()
  private let backgroundImageShadowView = UIView()
  private let backgroundGradientLayer = CAGradientLayer()
  private let ribbonImageView = UIImageView(image: UIImage(resource: .crowdfundingRibbon))
  private let actionLabelGradientLayer = CAGradientLayer()
  private let highlightOverlayView = UIView()
  private let feedbackGenerator = UIImpactFeedbackGenerator(style: .medium)
  private let onTap: () -> Void

  init(onTap: @escaping (() -> Void)) {
    self.onTap = onTap
    super.init(frame: .zero)
    setupViews()
    layoutViews()
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError()
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    backgroundGradientLayer.frame = backgroundImageShadowView.bounds
    actionLabelGradientLayer.frame = actionLabelBackgroundView.bounds
    highlightOverlayView.frame = backgroundImageView.bounds
  }

  private func setupViews() {
    addTarget(self, action: #selector(touchDown), for: .touchDown)
    addTarget(self, action: #selector(touchUpInside), for: .touchUpInside)
    addTarget(self, action: #selector(touchUpOutside), for: .touchUpOutside)

    descriptionLabel.textAlignment = .center
    descriptionLabel.lineBreakMode = .byWordWrapping
    descriptionLabel.numberOfLines = 0

    actionLabel.text = L("support_organic_maps")
    actionLabel.allowsDefaultTighteningForTruncation = true
    actionLabel.lineBreakMode = .byWordWrapping
    actionLabel.numberOfLines = 0
    actionLabel.textAlignment = .center
    actionLabel.isUserInteractionEnabled = false
    actionLabel.setStyle(.crowdfundingButton)

    actionLabelBackgroundView.backgroundColor = .ratingYellow
    actionLabelBackgroundView.layer.setCornerRadius(.buttonDefaultBig)
    actionLabelBackgroundView.clipsToBounds = true
    actionLabelBackgroundView.isUserInteractionEnabled = false

    actionLabelGradientLayer.startPoint = CGPoint(x: 0, y: 0)
    actionLabelGradientLayer.endPoint = CGPoint(x: 1, y: 1)
    actionLabelGradientLayer.frame = actionLabelBackgroundView.bounds
    let actionLabelGradientColor = UIColor.ratingYellow
    actionLabelGradientLayer.colors = [
      actionLabelGradientColor.lighter(percent: Constants.gradientColorMultiplier).cgColor,
      actionLabelGradientColor.darker(percent: Constants.gradientColorMultiplier).cgColor,
    ]
    actionLabelBackgroundView.layer.insertSublayer(actionLabelGradientLayer, at: 0)

    backgroundImageView.backgroundColor = UIColor(patternImage: UIImage(resource: .crowdfundingPattern))
    backgroundImageView.layer.setCornerRadius(.buttonDefaultBig)
    backgroundImageView.clipsToBounds = true
    backgroundImageView.isUserInteractionEnabled = false

    backgroundImageShadowView.layer.cornerRadius = backgroundImageView.layer.cornerRadius
    backgroundImageShadowView.layer.shadowColor = UIColor.black.cgColor
    backgroundImageShadowView.layer.shadowRadius = 6
    backgroundImageShadowView.layer.shadowOpacity = 0.4
    backgroundImageShadowView.layer.shadowOffset = .zero
    backgroundImageShadowView.isUserInteractionEnabled = false

    backgroundGradientLayer.cornerRadius = backgroundImageView.layer.cornerRadius
    let backgroundGradientColor = UIColor(fromHexString: Constants.backgroundColorHex)
    backgroundGradientLayer.colors = [
      backgroundGradientColor.lighter(percent: Constants.gradientColorMultiplier).cgColor,
      backgroundGradientColor.darker(percent: Constants.gradientColorMultiplier).cgColor,
    ]
    backgroundImageShadowView.layer.insertSublayer(backgroundGradientLayer, at: 0)

    ribbonImageView.contentMode = .scaleAspectFit
    ribbonImageView.isUserInteractionEnabled = false

    descriptionLabel.text = L("donate_description")
    descriptionLabel.setFontStyleAndApply(.semibold16, color: .whitePrimary)
    descriptionLabel.layer.shadowColor = UIColor.black.cgColor
    descriptionLabel.layer.shadowRadius = 6
    descriptionLabel.layer.shadowOpacity = 1.0
    descriptionLabel.layer.shadowOffset = .zero

#if DEBUG
    let longPressGesture = UILongPressGestureRecognizer(target: self, action: #selector(didLongPressDescription))
    descriptionLabel.addGestureRecognizer(longPressGesture)
    descriptionLabel.isUserInteractionEnabled = true
#endif

    highlightOverlayView.isUserInteractionEnabled = false
    highlightOverlayView.backgroundColor = UIColor.black.withAlphaComponent(Constants.highlightAlpha)
    highlightOverlayView.alpha = 0
    backgroundImageView.addSubview(highlightOverlayView)

    isAccessibilityElement = true
    accessibilityTraits = [.button]
    accessibilityLabel = L("support_organic_maps")
  }

  private func layoutViews() {
    addSubview(backgroundImageView)
    addSubview(ribbonImageView)
    addSubview(descriptionLabel)
    addSubview(actionLabelBackgroundView)
    actionLabelBackgroundView.addSubview(actionLabel)
    insertSubview(backgroundImageShadowView, belowSubview: backgroundImageView)

    backgroundImageView.translatesAutoresizingMaskIntoConstraints = false
    ribbonImageView.translatesAutoresizingMaskIntoConstraints = false
    backgroundImageShadowView.translatesAutoresizingMaskIntoConstraints = false
    descriptionLabel.translatesAutoresizingMaskIntoConstraints = false
    actionLabelBackgroundView.translatesAutoresizingMaskIntoConstraints = false
    actionLabel.translatesAutoresizingMaskIntoConstraints = false
    actionLabelBackgroundView.setContentCompressionResistancePriority(.required, for: .vertical)
    actionLabel.setContentCompressionResistancePriority(.required, for: .vertical)

    NSLayoutConstraint.activate([
      backgroundImageView.leadingAnchor.constraint(equalTo: leadingAnchor),
      backgroundImageView.trailingAnchor.constraint(equalTo: trailingAnchor),
      backgroundImageView.topAnchor.constraint(equalTo: topAnchor),
      backgroundImageView.bottomAnchor.constraint(equalTo: bottomAnchor),

      backgroundImageShadowView.leadingAnchor.constraint(equalTo: backgroundImageView.leadingAnchor),
      backgroundImageShadowView.trailingAnchor.constraint(equalTo: backgroundImageView.trailingAnchor),
      backgroundImageShadowView.topAnchor.constraint(equalTo: backgroundImageView.topAnchor),
      backgroundImageShadowView.bottomAnchor.constraint(equalTo: backgroundImageView.bottomAnchor),

      ribbonImageView.trailingAnchor.constraint(equalTo: backgroundImageView.trailingAnchor),
      ribbonImageView.topAnchor.constraint(equalTo: backgroundImageView.topAnchor),
      ribbonImageView.widthAnchor.constraint(equalToConstant: Constants.ribbonSize.width),
      ribbonImageView.heightAnchor.constraint(equalToConstant: Constants.ribbonSize.height),

      descriptionLabel.leadingAnchor.constraint(equalTo: leadingAnchor, constant: Constants.contentInsets.left),
      descriptionLabel.trailingAnchor.constraint(equalTo: trailingAnchor, constant: Constants.contentInsets.right),
      descriptionLabel.topAnchor.constraint(equalTo: topAnchor, constant: Constants.contentInsets.top),

      actionLabelBackgroundView.topAnchor.constraint(equalTo: descriptionLabel.bottomAnchor, constant: Constants.actionButtonSpacing),
      actionLabelBackgroundView.widthAnchor.constraint(greaterThanOrEqualToConstant: Constants.actionButtonMinWidth).withPriority(.defaultHigh),
      actionLabelBackgroundView.widthAnchor.constraint(lessThanOrEqualToConstant: Constants.actionButtonMaxWidth).withPriority(.defaultHigh),
      actionLabelBackgroundView.widthAnchor.constraint(lessThanOrEqualTo: backgroundImageView.widthAnchor, constant: -Constants.actionButtonSpacing * 2),
      actionLabelBackgroundView.centerXAnchor.constraint(equalTo: centerXAnchor),
      actionLabelBackgroundView.heightAnchor.constraint(greaterThanOrEqualToConstant: Constants.actionButtonHeight),
      actionLabelBackgroundView.bottomAnchor.constraint(equalTo: bottomAnchor, constant: Constants.contentInsets.bottom),

      actionLabel.leadingAnchor.constraint(equalTo: actionLabelBackgroundView.leadingAnchor, constant: Constants.actionButtonTitleInsets.left),
      actionLabel.trailingAnchor.constraint(equalTo: actionLabelBackgroundView.trailingAnchor, constant: -Constants.actionButtonTitleInsets.right),
      actionLabel.topAnchor.constraint(equalTo: actionLabelBackgroundView.topAnchor, constant: Constants.actionButtonTitleInsets.top),
      actionLabel.bottomAnchor.constraint(equalTo: actionLabelBackgroundView.bottomAnchor, constant: -Constants.actionButtonTitleInsets.bottom),
    ])
  }

  @objc
  private func didLongPressDescription() {
#if DEBUG
    Settings.resetDonations()
    Toast.show(withText: "Donations statistics was reset.", alignment: .top)
#endif
  }

  @objc
  private func touchDown() {
    let animations = { [weak self] in
      guard let self else { return }
      self.transform = CGAffineTransform(scaleX: Constants.pressScale, y: Constants.pressScale)
      self.highlightOverlayView.alpha = 1
      self.backgroundImageShadowView.layer.shadowOpacity = 0.5
      self.feedbackGenerator.impactOccurred()
    }
    UIView.animate(
      withDuration: Constants.tapAnimationDuration,
      delay: 0,
      options: [.beginFromCurrentState, .curveEaseInOut],
      animations: animations
    )
  }

  @objc
  private func touchUpInside() {
    onTap()
    touchUpOutside()
  }

  @objc
  private func touchUpOutside() {
    let animations = { [weak self] in
      guard let self else { return }
      self.transform = .identity
      self.highlightOverlayView.alpha = 0
      self.backgroundImageShadowView.layer.shadowOpacity = 0.3
    }
    UIView.animate(
      withDuration: Constants.tapAnimationDuration,
      delay: 0,
      options: [.beginFromCurrentState, .curveEaseInOut],
      animations: animations
    )
  }
}
