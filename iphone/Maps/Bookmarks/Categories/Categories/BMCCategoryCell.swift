protocol BMCCategoryCellDelegate {
  func visibilityAction(category: BMCCategory)
  func moreAction(category: BMCCategory, anchor: UIView)
}

final class BMCCategoryCell: MWMTableViewCell {
  @IBOutlet private weak var accessImageView: UIImageView!
  @IBOutlet private weak var visibility: UIButton!
  @IBOutlet private weak var titleLabel: UILabel! {
    didSet {
      titleLabel.font = .regular16()
      titleLabel.textColor = .blackPrimaryText()
    }
  }

  @IBOutlet private weak var subtitleLabel: UILabel! {
    didSet {
      subtitleLabel.font = .regular14()
      subtitleLabel.textColor = .blackSecondaryText()
    }
  }

  @IBOutlet private weak var moreButton: UIButton! {
    didSet {
      moreButton.tintColor = .blackSecondaryText()
      moreButton.setImage(#imageLiteral(resourceName: "ic24PxMore"), for: .normal)
    }
  }

  private var category: BMCCategory? {
    willSet {
      category?.removeObserver(self)
    }
    didSet {
      categoryUpdated()
      category?.addObserver(self)
    }
  }

  private var delegate: BMCCategoryCellDelegate?

  func config(category: BMCCategory, delegate: BMCCategoryCellDelegate) -> UITableViewCell {
    self.category = category
    self.delegate = delegate
    return self
  }

  @IBAction private func visibilityAction() {
    guard let category = category else { return }
    delegate?.visibilityAction(category: category)
  }

  @IBAction private func moreAction() {
    guard let category = category else { return }
    delegate?.moreAction(category: category, anchor: moreButton)
  }
}

extension BMCCategoryCell: BMCCategoryObserver {
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
    case .other:
      assert(false, "We don't expect category with .other status here")
      accessImageView.image = nil
      accessString = ""
    }

    let placesString = String(format: L("bookmarks_places"), category.count)
    subtitleLabel.text = accessString.count > 0 ? "\(accessString) • \(placesString)" : placesString

    if category.isVisible {
      visibility.tintColor = .linkBlue()
      visibility.setImage(#imageLiteral(resourceName: "radioBtnOn"), for: .normal)
      visibility.imageView?.mwm_coloring = .blue
    } else {
      visibility.tintColor = .blackHintText()
      visibility.setImage(#imageLiteral(resourceName: "radioBtnOff"), for: .normal)
      visibility.imageView?.mwm_coloring = .gray
    }
  }
}
