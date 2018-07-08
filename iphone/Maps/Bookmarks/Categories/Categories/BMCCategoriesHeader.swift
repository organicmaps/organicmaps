protocol BMCCategoriesHeaderDelegate {
  func visibilityAction(_ categoriesHeader: BMCCategoriesHeader)
}

final class BMCCategoriesHeader: UITableViewHeaderFooterView {
  @IBOutlet private weak var label: UILabel! {
    didSet {
      label.font = .medium14()
      label.textColor = .blackSecondaryText()
      label.text = L("bookmarks_groups").uppercased()
    }
  }

  @IBOutlet private weak var button: UIButton! {
    didSet {
      button.setTitleColor(.linkBlue(), for: .normal)
    }
  }

  var isShowAll = false {
    didSet {
      let title = L(isShowAll ? "bookmarks_groups_show_all" : "bookmarks_groups_hide_all")
      UIView.performWithoutAnimation {
        button.setTitle(title, for: .normal)
        button.layoutIfNeeded()
      }
    }
  }

  var delegate: BMCCategoriesHeaderDelegate!

  @IBAction private func buttonAction() {
    delegate.visibilityAction(self)
  }
}
