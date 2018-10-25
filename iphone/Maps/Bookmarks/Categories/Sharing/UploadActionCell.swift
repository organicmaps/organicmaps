final class UploadActionCell: MWMTableViewCell  {
  @IBOutlet private weak var actionImage: UIImageView! {
    didSet {
      actionImage.tintColor = .linkBlue()
    }
  }
  
  @IBOutlet private weak var actionTitle: UILabel!
  
  func config(title: String, image: UIImage) {
    actionImage.image = image
    actionTitle.text = title
  }
}
