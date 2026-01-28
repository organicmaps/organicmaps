final class ExpandableLabel: UIView {
  typealias TapHandler = () -> Void

  private enum Constants {
    static let defaultContentInsets = UIEdgeInsets(top: 8, left: 16, bottom: -8, right: -16)
  }

  private let stackView = UIStackView()
  private let textView = UITextView()
  private let expandLabel = UILabel()
  private var contentInsets: UIEdgeInsets = Constants.defaultContentInsets

  private var containerText: String?
  private var containerMaximumNumberOfLines = 2 {
    didSet {
      textView.textContainer.maximumNumberOfLines = containerMaximumNumberOfLines
      textView.invalidateIntrinsicContentSize()
    }
  }
  private var oldWidth: CGFloat = 0

  // MARK: - Public properties

  var didTap: TapHandler?
  var didLongPress: TapHandler?

  var font = UIFont.regular14() {
    didSet {
      textView.font = font
      expandLabel.font = font
    }
  }

  var textStyle: TextColorStyleSheet = .blackPrimary {
    didSet {
      textView.setStyle(textStyle)
    }
  }

  var expandStyle: TextColorStyleSheet = .linkBlue {
    didSet {
      expandLabel.setStyle(expandStyle)
    }
  }

  var text: String? {
    didSet {
      containerText = text
      textView.text = text
      if let text = text {
        isHidden = text.isEmpty
      } else {
        isHidden = true
      }
    }
  }

  var attributedText: NSAttributedString? {
    didSet {
      containerText = attributedText?.string
      textView.attributedText = attributedText
      if let attributedText = attributedText {
        isHidden = attributedText.length == 0
      } else {
        isHidden = true
      }
    }
  }

  var expandText = L("text_more_button") {
    didSet {
      expandLabel.text = expandText
    }
  }

  var numberOfLines = 2 {
    didSet {
      containerMaximumNumberOfLines = numberOfLines > 0 ? numberOfLines + 1 : 0
    }
  }

  // MARK: - Init

  override func setContentHuggingPriority(_ priority: UILayoutPriority, for axis: NSLayoutConstraint.Axis) {
    super.setContentHuggingPriority(priority, for: axis)
    textView.setContentHuggingPriority(priority, for: axis)
    expandLabel.setContentHuggingPriority(priority, for: axis)
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
    stackView.axis = .vertical
    stackView.alignment = .leading

    containerMaximumNumberOfLines = numberOfLines > 0 ? numberOfLines + 1 : 0

    textView.textContainer.lineFragmentPadding = 0
    textView.isScrollEnabled = false
    textView.isEditable = false
    textView.textContainerInset = .zero
    textView.contentMode = .topLeft
    textView.font = font
    textView.setStyle(textStyle)
    textView.text = text
    textView.attributedText = attributedText
    textView.setContentHuggingPriority(contentHuggingPriority(for: .vertical), for: .vertical)
    textView.backgroundColor = .clear
    textView.dataDetectorTypes = [.link, .phoneNumber]

    expandLabel.setContentHuggingPriority(contentHuggingPriority(for: .vertical), for: .vertical)
    expandLabel.font = font
    expandLabel.setStyle(expandStyle)
    expandLabel.text = expandText

    let tapGesture = UITapGestureRecognizer(target: self, action: #selector(onTap))
    addGestureRecognizer(tapGesture)

    layoutView()
  }

  private func layoutView() {
    addSubview(stackView)
    stackView.addArrangedSubview(textView)
    stackView.addArrangedSubview(expandLabel)
    stackView.alignToSuperview(contentInsets)
  }

  // MARK: - Actions

  @objc
  private func onTap(_ sender: UITapGestureRecognizer) {
    guard sender.state == .ended else { return }
    didTap?()
  }

  // MARK: - Public methods

  func setExpanded(_ expanded: Bool) {
    guard expandLabel.isHidden != expanded else { return }
    UIView.animate(withDuration: kFastAnimationDuration) {
      self.containerMaximumNumberOfLines = expanded ? 0 : (self.numberOfLines > 0 ? self.numberOfLines + 1 : 0)
      self.expandLabel.isHidden = expanded ? true : false
      self.stackView.layoutIfNeeded()
    }
  }

  override func layoutSubviews() {
    super.layoutSubviews()

    if oldWidth != bounds.width, let attributedText = attributedText?.mutableCopy() as? NSMutableAttributedString {
      attributedText.enumerateAttachments(estimatedWidth: bounds.width)
      self.attributedText = attributedText
      oldWidth = bounds.width
    }

    guard containerMaximumNumberOfLines > 0,
      containerMaximumNumberOfLines != numberOfLines,
      let s = containerText,
      !s.isEmpty else {
        return
    }
    let textRect = s.boundingRect(with: CGSize(width: width, height: CGFloat.greatestFiniteMagnitude),
                                  options: .usesLineFragmentOrigin,
                                  attributes: [.font: font],
                                  context: nil)
    let lineHeight = font.lineHeight
    if Int(lineHeight * CGFloat(numberOfLines + 1)) >= Int(textRect.height) {
      expandLabel.isHidden = true
      containerMaximumNumberOfLines = 0
    } else {
      expandLabel.isHidden = false
      containerMaximumNumberOfLines = numberOfLines
    }
    layoutIfNeeded()
  }
}
