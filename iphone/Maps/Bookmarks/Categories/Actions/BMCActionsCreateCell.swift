final class BMCActionsCreateCell: MWMTableViewCell {
  @IBOutlet private weak var actionImage: UIImageView! {
    didSet {
      actionImage.tintColor = .linkBlue()
    }
  }

  @IBOutlet private weak var actionTitle: UILabel! {
    didSet {
      actionTitle.font = .regular16()
      actionTitle.textColor = .blackPrimaryText()
    }
  }

  private var model: BMCAction! {
    didSet {
      switch model! {
      case .create:
        actionImage.image = #imageLiteral(resourceName: "ic24PxAddCopy")
        actionTitle.text = L("bookmarks_create_new_group")
      }
    }
  }

  func config(model: BMCAction) -> UITableViewCell {
    self.model = model
    return self
  }
}
