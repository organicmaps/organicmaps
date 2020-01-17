final class RouteManagerHeaderView: UIView {
  @IBOutlet private weak var titleLabel: UILabel!
  @IBOutlet private weak var addLocationButton: UIButton! {
    didSet {
// TODO(igrechuhin): Uncomment when start_from_my_position translation is ready.
//      addLocationButton.setTitle(L("start_from_my_position"), for: .normal)
//      addLocationButton.setTitleColor(UIColor.linkBlue(), for: .normal)
//      addLocationButton.setTitleColor(UIColor.buttonDisabledBlueText(), for: .disabled)
//      addLocationButton.tintColor = UIColor.linkBlue()
//
//      let flipLR = CGAffineTransform(scaleX: -1.0, y: 1.0)
//      addLocationButton.transform = flipLR
//      addLocationButton.titleLabel?.transform = flipLR
//      addLocationButton.imageView?.transform = flipLR
    }
  }
  @IBOutlet weak var separator: UIView!

  var isLocationButtonEnabled = true {
    didSet {
      addLocationButton.isEnabled = isLocationButtonEnabled
      addLocationButton.tintColor = isLocationButtonEnabled ? UIColor.linkBlue() : UIColor.buttonDisabledBlueText()
    }
  }

  override func awakeFromNib() {
    super.awakeFromNib()
  }

  override func applyTheme() {
    super.applyTheme()
    addLocationButton.tintColor = isLocationButtonEnabled ? UIColor.linkBlue() : UIColor.buttonDisabledBlueText()
  }
}
