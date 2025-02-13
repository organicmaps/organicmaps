final class SearchHistoryCell: MWMTableViewCell {
  enum Content {
    case query(String)
    case clear
  }

  static private let placeholderImage = UIImage.filled(with: .clear, size: CGSize(width: 28, height: 28))

  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: .default, reuseIdentifier: reuseIdentifier)
    setStyle(.defaultTableViewCell)
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  func configure(for content: Content) {
    switch content {
    case .query(let query):
      textLabel?.text = query
      textLabel?.setFontStyleAndApply(.regular17, color: .blackSecondary)
      imageView?.image = UIImage(resource: .icSearch)
      imageView?.setStyleAndApply(.black)
      isSeparatorHidden = false
    case .clear:
      textLabel?.text = L("clear_search")
      textLabel?.setFontStyleAndApply(.regular14, color: .linkBlue)
      imageView?.image = Self.placeholderImage
      isSeparatorHidden = true
    }
  }
}
