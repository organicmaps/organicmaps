class CatalogCategoryCell: UITableViewCell {

  @IBOutlet weak var visibleCheckmark: Checkmark! {
    didSet {
      visibleCheckmark.offTintColor = .blackHintText()
      visibleCheckmark.onTintColor = .linkBlue()
    }
  }
  @IBOutlet weak var titleLabel: UILabel! {
    didSet {
      titleLabel.font = .regular16()
      titleLabel.textColor = .blackPrimaryText()
    }
  }
  @IBOutlet weak var subtitleLabel: UILabel! {
    didSet {
      subtitleLabel.font = .regular14()
      subtitleLabel.textColor = .blackSecondaryText()
    }
  }
  @IBOutlet weak var moreButton: UIButton!

  @IBAction func onVisibleChanged(_ sender: Checkmark) {
  }
  @IBAction func onMoreButton(_ sender: UIButton) {
  }

  func update(with category: MWMCatalogCategory) {
    titleLabel.text = category.title
    subtitleLabel.text = "\(category.bookmarksCount) places • by \(category.author ?? "")"
    visibleCheckmark.isChecked = category.isVisible
  }
}
