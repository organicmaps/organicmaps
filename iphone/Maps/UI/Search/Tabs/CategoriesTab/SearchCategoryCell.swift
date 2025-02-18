final class SearchCategoryCell: UITableViewCell {

  private var categoryName: String = ""

  override init(style: UITableViewCell.CellStyle, reuseIdentifier: String?) {
    super.init(style: .default, reuseIdentifier: reuseIdentifier)
    setStyle(.defaultTableViewCell)
  }

  @available(*, unavailable)
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  func configure(with categoryName: String) {
    self.categoryName = categoryName
    textLabel?.text = L(categoryName)
    imageView?.mwm_name = String(format: "ic_%@", categoryName)
  }

  override func applyTheme() {
    super.applyTheme()
    imageView?.mwm_name = String(format: "ic_%@", categoryName)
  }
}
