final class BMCActionsCell: MWMTableViewCell {
  @IBOutlet private weak var actionImage: UIImageView!

  @IBOutlet private weak var actionTitle: UILabel!

  private var model: BMCAction! {
    didSet {
      actionImage.image = model.image
      actionTitle.text = model.title
    }
  }

  func config(model: BMCAction) -> UITableViewCell {
    self.model = model
    return self
  }
}
