enum ExpandableText: Equatable {
  case html(NSAttributedString)
  case plain(String)

  static func == (lhs: ExpandableText, rhs: ExpandableText) -> Bool {
    switch (lhs, rhs) {
    case (.html(let lhsText), .html(let rhsText)):
      return lhsText.isEqual(to: rhsText)
    case (.plain(let lhsText), .plain(let rhsText)):
      return lhsText == rhsText
    default:
      return false
    }
  }
}

final class ExpandableLabel: UIView {
  typealias TapHandler = () -> Void

  private enum Constants {
    static let defaultContentInsets = UIEdgeInsets(top: 8, left: 16, bottom: -8, right: -16)
    static let htmlCompactHeight: CGFloat = 70
    static let plainTextCollapsedNumberOfLines = 3
  }

  private let stackView = UIStackView()
  private let textContainerView = UIView()
  private let textView = UITextView()
  private let expandView = ExpandView()
  private let tapGestureRecognizer = UITapGestureRecognizer()
  private var contentInsets: UIEdgeInsets = Constants.defaultContentInsets
  private var textContainerHeightConstraint: NSLayoutConstraint!
  private var textViewHeightConstraint: NSLayoutConstraint!
  private var isExpanded = false
  private var sourceAttributedText: NSAttributedString?
  private var isMeasuring = false
  private var oldWidth: CGFloat = 0
  private var font = UIFont.regular14()
  private var textStyle: TextColorStyleSheet = .blackPrimary
  private var expandStyle: TextColorStyleSheet = .linkBlue
  private var expandText = L("text_more_button")

  // MARK: - Public properties

  var didTap: TapHandler?
  var expandableText: ExpandableText? {
    didSet {
      guard expandableText != oldValue else { return }
      oldWidth = 0
      applyExpandableText()
      setNeedsLayout()
    }
  }

  // MARK: - Init

  init(contentInsets: UIEdgeInsets = Constants.defaultContentInsets) {
    self.contentInsets = contentInsets
    super.init(frame: .zero)
    setupView()
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    super.init(coder: coder)
    setupView()
  }

  // MARK: - Private methods

  private func setupView() {
    clipsToBounds = true

    stackView.distribution = .fillProportionally
    stackView.axis = .vertical
    stackView.alignment = .fill
    stackView.clipsToBounds = true

    textContainerView.backgroundColor = .clear
    textContainerView.clipsToBounds = false

    textView.textContainer.lineFragmentPadding = 0
    textView.isScrollEnabled = false
    textView.isEditable = false
    textView.textContainerInset = .zero
    textView.contentMode = .topLeft
    textView.textContainer.maximumNumberOfLines = 0
    textView.font = font
    textView.setStyle(textStyle)
    textView.setContentHuggingPriority(.defaultHigh, for: .vertical)
    textView.backgroundColor = .clear
    textView.isSelectable = true
    textView.dataDetectorTypes = [.link, .phoneNumber]
    textView.clipsToBounds = false

    expandView.isHidden = true
    expandView.setContentHuggingPriority(.defaultHigh, for: .vertical)
    expandView.updateAppearance()
    expandView.label.font = font
    expandView.label.setStyle(expandStyle)
    expandView.label.text = expandText

    tapGestureRecognizer.addTarget(self, action: #selector(onTap))
    tapGestureRecognizer.cancelsTouchesInView = false
    tapGestureRecognizer.delegate = self
    addGestureRecognizer(tapGestureRecognizer)

    applyExpandableText()
    layoutView()
  }

  private func layoutView() {
    addSubview(stackView)
    stackView.addArrangedSubview(textContainerView)
    stackView.addArrangedSubview(expandView)
    stackView.alignToSuperview(contentInsets)

    textContainerView.addSubview(textView)
    textView.translatesAutoresizingMaskIntoConstraints = false

    textContainerHeightConstraint = textContainerView.heightAnchor.constraint(equalToConstant: Constants.htmlCompactHeight)
    textViewHeightConstraint = textView.heightAnchor.constraint(equalToConstant: Constants.htmlCompactHeight)

    NSLayoutConstraint.activate([
      textView.topAnchor.constraint(equalTo: textContainerView.topAnchor),
      textView.leadingAnchor.constraint(equalTo: textContainerView.leadingAnchor),
      textView.trailingAnchor.constraint(equalTo: textContainerView.trailingAnchor),
      textContainerHeightConstraint,
      textViewHeightConstraint,
    ])
  }

  private func updateCollapsedState() {
    let measuringWidth = bounds.width - contentInsets.left + contentInsets.right
    guard measuringWidth > 0 else { return }

    let fullHeight = measureFullHeight(for: measuringWidth)
    let collapsedHeight = measureCollapsedHeight(for: measuringWidth)
    let canExpand = fullHeight > collapsedHeight
    let shouldCollapse = !isExpanded && canExpand

    textViewHeightConstraint.constant = fullHeight
    textContainerHeightConstraint.constant = shouldCollapse ? collapsedHeight : fullHeight
    expandView.isHidden = !shouldCollapse
    textView.isUserInteractionEnabled = isExpanded || !canExpand
  }

  private func measureFullHeight(for width: CGFloat) -> CGFloat {
    measureHeight(for: width, maximumNumberOfLines: 0)
  }

  private func measureCollapsedHeight(for width: CGFloat) -> CGFloat {
    switch expandableText {
    case .html:
      return Constants.htmlCompactHeight
    case .plain:
      return measureHeight(for: width, maximumNumberOfLines: Constants.plainTextCollapsedNumberOfLines)
    case .none:
      return Constants.htmlCompactHeight
    }
  }

  private func measureHeight(for width: CGFloat, maximumNumberOfLines: Int) -> CGFloat {
    isMeasuring = true
    defer { isMeasuring = false }

    let previousMaximumNumberOfLines = textView.textContainer.maximumNumberOfLines
    textView.textContainer.maximumNumberOfLines = maximumNumberOfLines
    let size = textView.sizeThatFits(CGSize(width: width, height: .greatestFiniteMagnitude))
    textView.textContainer.maximumNumberOfLines = previousMaximumNumberOfLines
    return size.height
  }

  private func applyExpandableText() {
    switch expandableText {
    case .html(let attributedText):
      sourceAttributedText = attributedText
      textView.text = nil
      textView.attributedText = attributedText
      isHidden = attributedText.length == 0
    case .plain(let text):
      sourceAttributedText = nil
      textView.attributedText = nil
      textView.text = text
      isHidden = text.isEmpty
    case .none:
      sourceAttributedText = nil
      textView.attributedText = nil
      textView.text = nil
      isHidden = true
    }
  }

  // MARK: - Actions

  @objc
  private func onTap(_ sender: UITapGestureRecognizer) {
    guard sender.state == .ended else { return }
    didTap?()
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
    UIView.animate(withDuration: kFastAnimationDuration) {
      self.updateCollapsedState()
      self.superview?.layoutIfNeeded()
    }
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    guard !isMeasuring else { return }

    if oldWidth != bounds.width {
      oldWidth = bounds.width
      if let source = sourceAttributedText {
        let mutableText = NSMutableAttributedString(attributedString: source)
        mutableText.enumerateAttachments(estimatedWidth: bounds.width)
        textView.attributedText = mutableText
      }
    }

    updateCollapsedState()
  }
}

extension ExpandableLabel: UIGestureRecognizerDelegate {
  func gestureRecognizer(_ gestureRecognizer: UIGestureRecognizer, shouldReceive touch: UITouch) -> Bool {
    guard gestureRecognizer === tapGestureRecognizer else { return true }
    return !isTouchOnInteractiveText(touch)
  }

  func gestureRecognizer(_: UIGestureRecognizer,
                         shouldRecognizeSimultaneouslyWith _: UIGestureRecognizer) -> Bool {
    true
  }

  /// Keep link taps inside UITextView and use the outer tap only for expand/collapse.
  private func isTouchOnInteractiveText(_ touch: UITouch) -> Bool {
    guard textView.isUserInteractionEnabled else { return false }

    let location = touch.location(in: textView)
    guard textView.bounds.contains(location) else { return false }

    let adjustedLocation = CGPoint(x: location.x - textView.textContainerInset.left,
                                   y: location.y - textView.textContainerInset.top)
    var fraction: CGFloat = 0
    let glyphIndex = textView.layoutManager.glyphIndex(for: adjustedLocation,
                                                       in: textView.textContainer,
                                                       fractionOfDistanceThroughGlyph: &fraction)
    guard fraction <= 1 else { return false }

    let glyphRect = textView.layoutManager.boundingRect(forGlyphRange: NSRange(location: glyphIndex, length: 1),
                                                        in: textView.textContainer)
    guard glyphRect.contains(adjustedLocation) else { return false }

    let characterIndex = textView.layoutManager.characterIndexForGlyph(at: glyphIndex)
    guard characterIndex < textView.textStorage.length else { return false }

    let attributes = textView.textStorage.attributes(at: characterIndex, effectiveRange: nil)
    return attributes[.link] != nil
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
    backgroundColor = .clear
    clipsToBounds = false
    layer.addSublayer(gradientLayer)
    addSubview(label)

    label.translatesAutoresizingMaskIntoConstraints = false
    label.setContentHuggingPriority(.defaultHigh, for: .vertical)
    NSLayoutConstraint.activate([
      label.leadingAnchor.constraint(equalTo: leadingAnchor),
      label.trailingAnchor.constraint(lessThanOrEqualTo: trailingAnchor),
      label.bottomAnchor.constraint(equalTo: bottomAnchor),
      label.topAnchor.constraint(greaterThanOrEqualTo: topAnchor),
    ])
  }
}
