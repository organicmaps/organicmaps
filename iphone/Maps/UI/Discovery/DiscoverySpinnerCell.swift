@objc(MWMDiscoverySpinnerCell)
final class DiscoverySpinnerCell: MWMTableViewCell {
  @IBOutlet private weak var spinner: UIImageView! {
    didSet {
      let postfix = UIColor.isNightMode() ? "_dark" : "_light"
      let animationImagesCount = 12
      var images = Array<UIImage>()
      for i in 1...animationImagesCount {
        images.append(UIImage(named: "Spinner_\(i)" + postfix)!)
      }
      spinner.animationDuration = 0.8
      spinner.animationImages = images
      spinner.startAnimating()
    }
  }

  override func prepareForReuse() {
    if !spinner.isAnimating {
      spinner.startAnimating()
    }
  }
}
