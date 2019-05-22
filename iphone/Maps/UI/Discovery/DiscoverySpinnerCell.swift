@objc(MWMDiscoverySpinnerCell)
final class DiscoverySpinnerCell: MWMTableViewCell {
  @IBOutlet private weak var spinner: UIImageView! {
    didSet {
      let postfix = UIColor.isNightMode() ? "_dark" : "_light"
      spinner.image = UIImage(named: "Spinner" + postfix)
      spinner.startRotation()
    }
  }

  override func prepareForReuse() {
    if !spinner.isRotating {
      spinner.startRotation()
    }
  }
}
