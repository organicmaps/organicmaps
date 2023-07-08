final class ExpandableLabel: UIView {
  typealias OnExpandClosure = (() -> Void) -> Void

  private let stackView = UIStackView()
  private let textView = UITextView()
  private let expandLabel = UILabel()

  var onExpandClosure: OnExpandClosure?

  var font = UIFont.systemFont(ofSize: 16) {
    didSet {
      textView.font = font
      expandLabel.font = font
    }
  }

  var textColor = UIColor.black {
    didSet {
      textView.textColor = textColor
    }
  }

  var expandColor = UIColor.systemBlue {
    didSet {
      expandLabel.textColor = expandColor
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

  var expandText = "More" {
    didSet {
      expandLabel.text = expandText
    }
  }

  var numberOfLines = 2 {
    didSet {
      containerMaximumNumberOfLines = numberOfLines > 0 ? numberOfLines + 1 : 0
    }
  }

  private var containerText: String?
  private var containerMaximumNumberOfLines = 2 {
    didSet {
      textView.textContainer.maximumNumberOfLines = containerMaximumNumberOfLines
      textView.invalidateIntrinsicContentSize()
    }
  }

  private var oldWidth: CGFloat = 0

  override func setContentHuggingPriority(_ priority: UILayoutPriority, for axis: NSLayoutConstraint.Axis) {
    super.setContentHuggingPriority(priority, for: axis)
    textView.setContentHuggingPriority(priority, for: axis)
    expandLabel.setContentHuggingPriority(priority, for: axis)
  }

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
    containerMaximumNumberOfLines = numberOfLines > 0 ? numberOfLines + 1 : 0
    textView.textContainer.lineFragmentPadding = 0
    textView.isScrollEnabled = false
    textView.isEditable = false
    textView.textContainerInset = .zero
    textView.contentMode = .topLeft
    textView.font = font
    textView.textColor = textColor
    textView.text = text
    textView.attributedText = attributedText
    textView.setContentHuggingPriority(contentHuggingPriority(for: .vertical), for: .vertical)
    textView.backgroundColor = .clear
    textView.dataDetectorTypes = [.link, .phoneNumber]
    expandLabel.setContentHuggingPriority(contentHuggingPriority(for: .vertical), for: .vertical)
    expandLabel.font = font
    expandLabel.textColor = expandColor
    expandLabel.text = expandText
    expandLabel.isHidden = true
    addSubview(stackView)

    stackView.addArrangedSubview(textView)
    stackView.addArrangedSubview(expandLabel)
    stackView.alignToSuperview()
    let gr = UITapGestureRecognizer(target: self, action: #selector(onExpand(_:)))
    addGestureRecognizer(gr)
  }

  @objc func onExpand(_ sender: UITapGestureRecognizer) {
    if expandLabel.isHidden { return }

    let expandClosure = {
      UIView.animate(withDuration: kDefaultAnimationDuration) {
        self.containerMaximumNumberOfLines = 0
        self.expandLabel.isHidden = true
        self.stackView.layoutIfNeeded()
      }
    }
    if let onExpandClosure = onExpandClosure {
      onExpandClosure(expandClosure)
    } else {
      expandClosure()
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
