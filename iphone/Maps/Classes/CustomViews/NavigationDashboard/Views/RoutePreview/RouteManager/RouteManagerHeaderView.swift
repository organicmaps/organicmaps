final class RouteManagerHeaderView: UIView {
  @IBOutlet private weak var titleLabel: UILabel! {
    didSet {
      titleLabel.text = L("planning_route_manage_route")
      titleLabel.font = UIFont.bold22()
      titleLabel.textColor = UIColor.blackPrimaryText()
    }
  }
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
  @IBOutlet weak var separator: UIView! {
    didSet {
      separator.backgroundColor = UIColor.blackDividers()
    }
  }

  var isLocationButtonEnabled = true {
    didSet {
      addLocationButton.isEnabled = isLocationButtonEnabled
      addLocationButton.tintColor = isLocationButtonEnabled ? UIColor.linkBlue() : UIColor.buttonDisabledBlueText()
    }
  }

  override func awakeFromNib() {
    super.awakeFromNib()
    backgroundColor = UIColor.white()
  }
}
