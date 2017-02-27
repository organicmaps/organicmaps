@objc (MWMPPFacilityCell)
final class PPFacilityCell: MWMTableViewCell {
  @IBOutlet private var facility: UILabel!

  func config(with str:String) {
    facility.text = str
  }
}
