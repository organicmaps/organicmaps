protocol BMCCategoryCellDelegate: AnyObject {
  func cell(_ cell: BMCCategoryCell, didCheck visible: Bool)
  func cell(_ cell: BMCCategoryCell, didPress moreButton: UIButton)
}

final class BMCCategoryCell: MWMTableViewCell {
  @IBOutlet private weak var accessImageView: UIImageView!
  @IBOutlet private weak var titleLabel: UILabel!
  @IBOutlet private weak var subtitleLabel: UILabel!

  @IBOutlet private weak var moreButton: UIButton! {
    didSet {
      moreButton.setImage(#imageLiteral(resourceName: "ic24PxMore"), for: .normal)
    }
  }

  @IBOutlet weak var visibleCheckmark: Checkmark!

  private var category: BookmarkGroup? {
    didSet {
      categoryUpdated()
    }
  }

  private weak var delegate: BMCCategoryCellDelegate?

  func config(category: BookmarkGroup, delegate: BMCCategoryCellDelegate) -> UITableViewCell {
    self.category = category
    self.delegate = delegate
    return self
  }

  @IBAction func onVisibleChanged(_ sender: Checkmark) {
    delegate?.cell(self, didCheck: sender.isChecked)
  }

  @IBAction private func moreAction() {
    delegate?.cell(self, didPress: moreButton)
  }

  func categoryUpdated() {
    guard let category = category else { return }
    titleLabel.text = category.title

    let accessString: String
    switch category.accessStatus {
    case .local:
      accessString = L("not_shared")
      accessImageView.image = UIImage(named: "ic_category_private")
    case .public:
      accessString = L("public_access")
      accessImageView.image = UIImage(named: "ic_category_public")
    case .private:
      accessString = L("limited_access")
      accessImageView.image = UIImage(named: "ic_category_link")
    case .authorOnly:
      accessString = L("access_rules_author_only")
      accessImageView.image = UIImage(named: "ic_category_private")
    case .other:
      assert(false, "We don't expect category with .other status here")
      accessImageView.image = nil
      accessString = ""
    }

    let placesString = category.placesCountTitle()
    subtitleLabel.text = accessString.count > 0 ? "\(accessString) • \(placesString)" : placesString
    visibleCheckmark.isChecked = category.isVisible
  }
}
