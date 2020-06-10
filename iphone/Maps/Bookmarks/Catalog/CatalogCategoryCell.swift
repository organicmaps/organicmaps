protocol CatalogCategoryCellDelegate : AnyObject {
  func cell(_ cell: CatalogCategoryCell, didCheck visible: Bool)
  func cell(_ cell: CatalogCategoryCell, didPress moreButton: UIButton)
}

final class CatalogCategoryCell: MWMTableViewCell {

  weak var delegate: CatalogCategoryCellDelegate?

  @IBOutlet weak var visibleCheckmark: Checkmark!
  @IBOutlet weak var titleLabel: UILabel!
  @IBOutlet weak var subtitleLabel: UILabel! 
  @IBOutlet weak var moreButton: UIButton!

  @IBAction func onVisibleChanged(_ sender: Checkmark) {
    delegate?.cell(self, didCheck: sender.isChecked)
  }

  @IBAction func onMoreButton(_ sender: UIButton) {
    delegate?.cell(self, didPress: sender)
  }

  func update(with category: BookmarkGroup, delegate: CatalogCategoryCellDelegate?) {
    titleLabel.text = category.title
    let placesString = category.placesCountTitle()
    let authorString = String(coreFormat: L("author_name_by_prefix"), arguments: [category.author])
    subtitleLabel.text = "\(placesString) • \(authorString)"
    visibleCheckmark.isChecked = category.isVisible
    self.delegate = delegate
  }
}
