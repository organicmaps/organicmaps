@objc(MWMFilterPriceCategoryCell)
final class FilterPriceCategoryCell: MWMTableViewCell {

  @IBOutlet private var priceButtons: [UIButton]! {
    didSet {
      priceButtons.map { $0.layer }.forEach {
        $0.cornerRadius = 4
        $0.borderWidth = 1
        $0.borderColor = UIColor.blackDividers().cgColor
      }
    }
  }

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
    backgroundColor = UIColor.clear
  }
}
