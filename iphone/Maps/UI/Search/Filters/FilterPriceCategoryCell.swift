@objc(MWMFilterPriceCategoryCell)
final class FilterPriceCategoryCell: MWMTableViewCell {

  @IBOutlet private var priceButtons: [UIButton]!
  @IBOutlet weak var one: UIButton!
  @IBOutlet weak var two: UIButton!
  @IBOutlet weak var three: UIButton!

  @IBAction private func tap(sender: UIButton!) {
    sender.isSelected = !sender.isSelected
  }

  override func awakeFromNib() {
    super.awakeFromNib()
    isSeparatorHidden = true
  }
}
