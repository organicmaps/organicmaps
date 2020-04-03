protocol BMCCategoriesHeaderDelegate: AnyObject {
  func visibilityAction(_ categoriesHeader: BMCCategoriesHeader)
}

final class BMCCategoriesHeader: UITableViewHeaderFooterView {
  @IBOutlet private weak var label: UILabel!
  @IBOutlet private weak var button: UIButton!

  var isShowAll = false {
    didSet {
      let title = L(isShowAll ? "bookmarks_groups_show_all" : "bookmarks_groups_hide_all")
      UIView.performWithoutAnimation {
        button.setTitle(title, for: .normal)
        button.layoutIfNeeded()
      }
    }
  }

  var title: String? {
    didSet {
      title = title?.uppercased()
      label.text = title
    }
  }

  weak var delegate: BMCCategoriesHeaderDelegate?

  @IBAction private func buttonAction() {
    delegate?.visibilityAction(self)
  }
}
