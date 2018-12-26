final class SearchCategoryCell: MWMTableViewCell {
  @IBOutlet weak var iconImageView: UIImageView!
  @IBOutlet weak var titleLabel: UILabel!
  
  func update(with category: String) {
    iconImageView.mwm_name = String(format: "ic_%@", category)
    titleLabel.text = L(category)
  }
}
