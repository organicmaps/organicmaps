protocol BMCCategoriesHeaderDelegate {
  func visibilityAction(isShowAll: Bool)
}

final class BMCCategoriesHeader: UIView {
  @IBOutlet private weak var label: UILabel! {
    didSet {
      label.font = .bold14()
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
      button.setTitle(title, for: .normal)
    }
  }

  var delegate: BMCCategoriesHeaderDelegate!

  @IBAction private func buttonAction() {
    delegate.visibilityAction(isShowAll: isShowAll)
  }
}
