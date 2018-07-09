@objc(MWMPPFacilityCell)
final class PPFacilityCell: MWMTableViewCell {
  @IBOutlet private var facility: UILabel!

  @objc func config(with str: String) {
    facility.text = str
  }
}
