@objc(MWMFilterPriceCategoryCell)
final class FilterPriceCategoryCell: MWMTableViewCell {

  @IBOutlet private var priceButtons: [UIButton]!
  @IBOutlet weak var one: UIButton!
  @IBOutlet weak var two: UIButton!
  @IBOutlet weak var three: UIButton!

  @IBAction private func tap(sender: UIButton!) {
    sender.isSelected = !sender.isSelected

    let priceCategory: String
    switch sender {
    case one: priceCategory = kStat1
    case two: priceCategory = kStat2
    case three: priceCategory = kStat3
    default:
      priceCategory = ""
      assert(false)
    }
    Statistics.logEvent(kStatSearchFilterClick, withParameters: [
      kStatCategory: kStatHotel,
      kStatPriceCategory: priceCategory,
    ])
  }

  override func awakeFromNib() {
    super.awakeFromNib()
    isSeparatorHidden = true
  }
}
