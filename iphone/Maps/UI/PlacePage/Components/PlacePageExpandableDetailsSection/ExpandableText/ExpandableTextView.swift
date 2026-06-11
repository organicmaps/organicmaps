final class ExpandableTextView: UITextView {
  private enum Constants {
    static let htmlTextCollapsedHeight: CGFloat = 70
    static let plainTextCollapsedNumberOfLines = 3
  }

  private var expandableText: ExpandableText
  private var sourceAttributedText: NSAttributedString?
  private var isMeasuring = false
  private var oldWidth: CGFloat = 0

  var contentView: UIView { self }
  var onContentHeightChanged: (() -> Void)?

  init(expandableText: ExpandableText) {
    self.expandableText = .plain("")
    super.init(frame: .zero, textContainer: nil)
    setupView()
    configure(with: expandableText)
  }

  override init(frame: CGRect, textContainer: NSTextContainer?) {
    expandableText = .plain("")
    super.init(frame: frame, textContainer: textContainer)
    setupView()
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    expandableText = .plain("")
    super.init(coder: coder)
    setupView()
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    onContentHeightChanged?()
  }

  private func setupView() {
    textContainer.lineFragmentPadding = 0
    isScrollEnabled = false
    isEditable = false
    textContainerInset = .zero
    contentMode = .topLeft
    textContainer.maximumNumberOfLines = 0
    setContentHuggingPriority(.defaultHigh, for: .vertical)
    backgroundColor = .clear
    isSelectable = true
    dataDetectorTypes = [.link, .phoneNumber]
    clipsToBounds = false
    translatesAutoresizingMaskIntoConstraints = false
    font = .regular14.dynamic
    adjustsFontForContentSizeCategory = true
    textColor = .blackPrimaryText
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    guard !isMeasuring else { return }

    if oldWidth != bounds.width {
      oldWidth = bounds.width
      updateAttributedTextForCurrentWidth()
    }
  }

  private func updateAttributedTextForCurrentWidth() {
    guard let sourceAttributedText else { return }

    let mutableText = NSMutableAttributedString(attributedString: sourceAttributedText)
    mutableText.enumerateAttachments(estimatedWidth: bounds.width)
    attributedText = mutableText
  }

  private static func buildAttributedString(from htmlString: String) -> NSAttributedString {
    let font = UIFont.regular14.dynamic
    let color = UIColor.blackPrimaryText
    let paragraphStyle = NSMutableParagraphStyle()
    paragraphStyle.lineSpacing = 4

    if let attributedString = NSMutableAttributedString(htmlString: htmlString,
                                                        baseFont: font,
                                                        paragraphStyle: paragraphStyle) {
      attributedString.addAttribute(.foregroundColor,
                                    value: color,
                                    range: NSRange(location: 0, length: attributedString.length))
      return attributedString
    }

    return NSAttributedString(string: htmlString,
                              attributes: [.font: font,
                                           .foregroundColor: color,
                                           .paragraphStyle: paragraphStyle])
  }
}

extension ExpandableTextView: ExpandableTextContainer {
  func configure(with text: String) {
    switch expandableText {
    case .html:
      configure(with: .html(text))
    case .plain:
      configure(with: .plain(text))
    }
  }

  private func configure(with expandableText: ExpandableText) {
    guard self.expandableText != expandableText else { return }

    self.expandableText = expandableText
    oldWidth = 0
    switch expandableText {
    case .html(let htmlString):
      let attributedText = Self.buildAttributedString(from: htmlString)
      sourceAttributedText = attributedText
      text = nil
      self.attributedText = attributedText
    case .plain(let string):
      sourceAttributedText = nil
      attributedText = nil
      text = string
    }
  }

  func expandedHeight(for width: CGFloat) -> CGFloat {
    measureHeight(for: width, maximumNumberOfLines: 0)
  }

  func collapsedHeight(for width: CGFloat) -> CGFloat {
    switch expandableText {
    case .html:
      return Constants.htmlTextCollapsedHeight
    case .plain:
      return measureHeight(for: width, maximumNumberOfLines: Constants.plainTextCollapsedNumberOfLines)
    }
  }

  private func measureHeight(for width: CGFloat, maximumNumberOfLines: Int) -> CGFloat {
    isMeasuring = true
    defer { isMeasuring = false }

    let previousMaximumNumberOfLines = textContainer.maximumNumberOfLines
    textContainer.maximumNumberOfLines = maximumNumberOfLines
    let size = sizeThatFits(CGSize(width: width, height: .greatestFiniteMagnitude))
    textContainer.maximumNumberOfLines = previousMaximumNumberOfLines
    return size.height
  }
}
