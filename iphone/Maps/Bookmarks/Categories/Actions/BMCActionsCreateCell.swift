final class BMCActionsCreateCell: MWMTableViewCell {
  @IBOutlet private weak var actionImage: UIImageView!

  @IBOutlet private weak var actionTitle: UILabel!

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
