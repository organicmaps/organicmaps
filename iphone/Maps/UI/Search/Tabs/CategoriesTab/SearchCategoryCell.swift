final class SearchCategoryCell: UITableViewCell {
  private var categoryName: String = ""

  override init(style _: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: .default, reuseIdentifier: reuseIdentifier)
    setStyle(.defaultTableViewCell)
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  func configure(with categoryName: String) {
    self.categoryName = categoryName
    textLabel?.text = L(categoryName)
    updateIcon()
  }

  override func applyTheme() {
    super.applyTheme()
    updateIcon()
  }

  private func updateIcon() {
    imageView?.image = SearchCategoryIconRenderer.image(for: categoryName,
                                                        userInterfaceStyle: traitCollection.userInterfaceStyle)
  }
}
