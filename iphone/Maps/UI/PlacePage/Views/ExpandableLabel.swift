final class ExpandableLabel: UIView {
  typealias OnExpandClosure = (() -> Void) -> Void
  
  private let stackView = UIStackView()
  private let textLabel = UILabel()
  private let expandLabel = UILabel()

  var onExpandClosure: OnExpandClosure?

  var font = UIFont.systemFont(ofSize: 16) {
    didSet {
      textLabel.font = font
      expandLabel.font = font
    }
  }

  var textColor = UIColor.black {
    didSet {
      textLabel.textColor = textColor
    }
  }

  var expandColor = UIColor.systemBlue {
    didSet {
      expandLabel.textColor = expandColor
    }
  }

  var text: String? {
    didSet {
      textLabel.text = text
      expandLabel.isHidden = true
    }
  }

  var attributedText: NSAttributedString? {
    didSet {
      textLabel.attributedText = attributedText
      expandLabel.isHidden = true
    }
  }

  var expandText = "More" {
    didSet {
      expandLabel.text = expandText
    }
  }

  var numberOfLines = 2 {
    didSet {
      textLabel.numberOfLines = numberOfLines > 0 ? numberOfLines + 1 : 0
    }
  }

  override func setContentHuggingPriority(_ priority: UILayoutPriority, for axis: NSLayoutConstraint.Axis) {
    super.setContentHuggingPriority(priority, for: axis)
    textLabel.setContentHuggingPriority(priority, for: axis)
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
    textLabel.numberOfLines = numberOfLines > 0 ? numberOfLines + 1 : 0
    textLabel.contentMode = .topLeft
    textLabel.font = font
    textLabel.textColor = textColor
    textLabel.text = text
    textLabel.attributedText = attributedText
    textLabel.setContentHuggingPriority(contentHuggingPriority(for: .vertical), for: .vertical)
    expandLabel.setContentHuggingPriority(contentHuggingPriority(for: .vertical), for: .vertical)
    expandLabel.font = font
    expandLabel.textColor = expandColor
    expandLabel.text = expandText
    expandLabel.isHidden = true
    addSubview(stackView)

    stackView.addArrangedSubview(textLabel)
    stackView.addArrangedSubview(expandLabel)
    stackView.alignToSuperview()
    let gr = UITapGestureRecognizer(target: self, action: #selector(onExpand(_:)))
    addGestureRecognizer(gr)
  }

  @objc func onExpand(_ sender: UITapGestureRecognizer) {
    if expandLabel.isHidden { return }
    
    let expandClosure = {
      UIView.animate(withDuration: kDefaultAnimationDuration) {
        self.textLabel.numberOfLines = 0
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

    guard textLabel.numberOfLines > 0, textLabel.numberOfLines != numberOfLines, let s = textLabel.text as NSString? else { return }
    let textRect = s.boundingRect(with: CGSize(width: width, height: CGFloat.greatestFiniteMagnitude),
                                  options: .usesLineFragmentOrigin,
                                  attributes: [.font: font],
                                  context: nil)
    let lineHeight = textLabel.font.lineHeight
    if Int(lineHeight * CGFloat(numberOfLines + 1)) >= Int(textRect.height) {
      expandLabel.isHidden = true
      textLabel.numberOfLines = 0
    } else {
      expandLabel.isHidden = false
      textLabel.numberOfLines = numberOfLines
    }
    layoutIfNeeded()
  }
}
