final class SearchCategoryCell: MWMTableViewCell {
  @IBOutlet weak var iconImageView: UIImageView!
  @IBOutlet weak var titleLabel: UILabel!

  private var category: String = ""
  func update(with category: String) {
    self.category = category
    iconImageView.mwm_name = String(format: "ic_%@", category)
    titleLabel.text = L(category)
  }

  override func applyTheme() {
    super.applyTheme()
    iconImageView.mwm_name = String(format: "ic_%@", category)
  }
}
