final class ExpandableLabel: UIView {
  typealias TapHandler = () -> Void

  private enum Constants {
    static let defaultContentInsets = UIEdgeInsets(top: 8, left: 16, bottom: -8, right: -16)
    static let defaultCompactHeight: CGFloat = 70
    static let expandControlHeight: CGFloat = 24
  }

  private final class ExpandView: UIView {
    private enum Constants {
      static let gradientOverlapHeight: CGFloat = 24
    }

    let label = UILabel()
    private let gradientLayer = CAGradientLayer()

    override init(frame: CGRect) {
      super.init(frame: frame)
      setupView()
    }

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

    func updateAppearance() {
      gradientLayer.startPoint = CGPoint(x: 0.5, y: 0)
      gradientLayer.endPoint = CGPoint(x: 0.5, y: 1)
      gradientLayer.colors = [
        UIColor.white.withAlphaComponent(0).cgColor,
        UIColor.white.withAlphaComponent(0.9).cgColor,
        UIColor.white.cgColor,
      ]
      gradientLayer.locations = [0, 0.7, 1]
    }

    private func setupView() {
      clipsToBounds = false
      layer.addSublayer(gradientLayer)
      addSubview(label)

      label.translatesAutoresizingMaskIntoConstraints = false

      NSLayoutConstraint.activate([
        label.leadingAnchor.constraint(equalTo: leadingAnchor),
        label.bottomAnchor.constraint(equalTo: bottomAnchor),
        label.topAnchor.constraint(greaterThanOrEqualTo: topAnchor),
      ])
    }
  }

  private let stackView = UIStackView()
  private let textView = UITextView()
  private let expandView = ExpandView()
  private var contentInsets: UIEdgeInsets = Constants.defaultContentInsets
  private var textHeightConstraint: NSLayoutConstraint!
  private var expandControlHeightConstraint: NSLayoutConstraint!
  private var isExpanded = false
  private var sourceAttributedText: NSAttributedString?
  private var isMeasuring = false
  private var oldWidth: CGFloat = 0

  // MARK: - Public properties

  var didTap: TapHandler?
  var didLongPress: TapHandler?

  var font = UIFont.regular14() {
    didSet {
      textView.font = font
      expandView.label.font = font
      setNeedsLayout()
    }
  }

  var textStyle: TextColorStyleSheet = .blackPrimary {
    didSet {
      textView.setStyle(textStyle)
    }
  }

  var expandStyle: TextColorStyleSheet = .linkBlue {
    didSet {
      expandView.label.setStyle(expandStyle)
    }
  }

  var text: String? {
    didSet {
      sourceAttributedText = nil
      textView.text = text
      if let text = text {
        isHidden = text.isEmpty
      } else {
        isHidden = true
      }
      setNeedsLayout()
    }
  }

  var attributedText: NSAttributedString? {
    didSet {
      sourceAttributedText = attributedText
      textView.attributedText = attributedText
      if let attributedText = attributedText {
        isHidden = attributedText.length == 0
      } else {
        isHidden = true
      }
      setNeedsLayout()
    }
  }

  var expandText = L("text_more_button") {
    didSet {
      expandView.label.text = expandText
    }
  }

  var compactHeight = Constants.defaultCompactHeight {
    didSet {
      textHeightConstraint.constant = compactHeight
      setNeedsLayout()
    }
  }

  // MARK: - Init

  override func setContentHuggingPriority(_ priority: UILayoutPriority, for axis: NSLayoutConstraint.Axis) {
    super.setContentHuggingPriority(priority, for: axis)
    textView.setContentHuggingPriority(priority, for: axis)
    expandView.setContentHuggingPriority(priority, for: axis)
  }

  override init(frame: CGRect) {
    super.init(frame: frame)
    setupView()
  }

  init(contentInsets: UIEdgeInsets = Constants.defaultContentInsets) {
    self.contentInsets = contentInsets
    super.init(frame: .zero)
    setupView()
  }

  required init?(coder: NSCoder) {
    super.init(coder: coder)
    setupView()
  }

  // MARK: - Private methods

  private func setupView() {
    clipsToBounds = true

    stackView.axis = .vertical
    stackView.alignment = .fill

    textView.textContainer.lineFragmentPadding = 0
    textView.isScrollEnabled = false
    textView.isEditable = false
    textView.textContainerInset = .zero
    textView.contentMode = .topLeft
    textView.textContainer.maximumNumberOfLines = 0
    textView.font = font
    textView.setStyle(textStyle)
    textView.text = text
    textView.attributedText = attributedText
    textView.setContentHuggingPriority(contentHuggingPriority(for: .vertical), for: .vertical)
    textView.backgroundColor = .clear
    textView.dataDetectorTypes = [.link, .phoneNumber]

    expandView.isHidden = true
    expandView.setContentHuggingPriority(contentHuggingPriority(for: .vertical), for: .vertical)
    expandView.updateAppearance()
    expandView.label.setContentHuggingPriority(contentHuggingPriority(for: .vertical), for: .vertical)
    expandView.label.font = font
    expandView.label.setStyle(expandStyle)
    expandView.label.text = expandText

    let tapGesture = UITapGestureRecognizer(target: self, action: #selector(onTap))
    stackView.addGestureRecognizer(tapGesture)

    layoutView()
  }

  private func layoutView() {
    addSubview(stackView)
    stackView.addArrangedSubview(textView)
    stackView.addArrangedSubview(expandView)
    stackView.alignToSuperview(contentInsets)

    textHeightConstraint = textView.heightAnchor.constraint(equalToConstant: compactHeight)
    expandControlHeightConstraint = expandView.heightAnchor.constraint(equalToConstant: 0)
    expandControlHeightConstraint.isActive = true
  }

  private func updateCollapsedState() {
    let measuringWidth = bounds.width - contentInsets.left + contentInsets.right
    guard measuringWidth > 0 else { return }

    let fullHeight = measureFullHeight(for: measuringWidth)
    let shouldCollapse = !isExpanded && fullHeight > compactHeight

    textHeightConstraint.isActive = shouldCollapse
    expandControlHeightConstraint.constant = shouldCollapse ? Constants.expandControlHeight : 0
    expandView.isHidden = !shouldCollapse
  }

  private func measureFullHeight(for width: CGFloat) -> CGFloat {
    isMeasuring = true
    defer { isMeasuring = false }
    let fullSize = textView.sizeThatFits(CGSize(width: width, height: .greatestFiniteMagnitude))
    return fullSize.height
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
      self.layoutIfNeeded()
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
