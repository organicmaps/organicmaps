protocol BMCCategoryCellDelegate {
  func visibilityAction(category: BMCCategory)
  func moreAction(category: BMCCategory, anchor: UIView)
}

final class BMCCategoryCell: MWMTableViewCell {
  @IBOutlet private weak var visibility: UIButton!
  @IBOutlet private weak var title: UILabel! {
    didSet {
      title.font = .regular16()
      title.textColor = .blackPrimaryText()
    }
  }

  @IBOutlet private weak var count: UILabel! {
    didSet {
      count.font = .regular14()
      count.textColor = .blackSecondaryText()
    }
  }

  @IBOutlet private weak var more: UIButton! {
    didSet {
      more.tintColor = .blackSecondaryText()
      more.setImage(#imageLiteral(resourceName: "ic24PxMore"), for: .normal)
    }
  }

  private var category: BMCCategory! {
    willSet {
      category?.removeObserver(self)
    }
    didSet {
      categoryUpdated()
      category?.addObserver(self)
    }
  }

  private var delegate: BMCCategoryCellDelegate!

  func config(category: BMCCategory, delegate: BMCCategoryCellDelegate) -> UITableViewCell {
    self.category = category
    self.delegate = delegate
    return self
  }

  @IBAction private func visibilityAction() {
    delegate.visibilityAction(category: category)
  }

  @IBAction private func moreAction() {
    delegate.moreAction(category: category, anchor: more)
  }
}

extension BMCCategoryCell: BMCCategoryObserver {
  func categoryUpdated() {
    title.text = category.title
    count.text = String(coreFormat: L("bookmarks_places"), arguments: [category.count])

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
