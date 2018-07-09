@objc(MWMFilterRatingCell)
final class FilterRatingCell: MWMTableViewCell {
  @IBOutlet private var ratingButtons: [UIButton]! {
    didSet {
      ratingButtons.map { $0.layer }.forEach {
        $0.cornerRadius = 4
        $0.borderWidth = 1
        $0.borderColor = UIColor.blackDividers().cgColor
      }
    }
  }

  @IBOutlet weak var any: UIButton!
  @IBOutlet weak var good: UIButton!
  @IBOutlet weak var veryGood: UIButton!
  @IBOutlet weak var excellent: UIButton!

  @IBAction private func tap(sender: UIButton!) {
    guard !sender.isSelected else { return }

    ratingButtons.forEach { $0.isSelected = false }
    sender.isSelected = true

    let rating: String
    switch sender {
    case any: rating = kStatAny
    case good: rating = kStat7
    case veryGood: rating = kStat8
    case excellent: rating = kStat9
    default:
      rating = ""
      assert(false)
    }
    Statistics.logEvent(kStatSearchFilterClick, withParameters: [
      kStatCategory: kStatHotel,
      kStatRating: rating,
    ])
  }

  override func awakeFromNib() {
    super.awakeFromNib()
    isSeparatorHidden = true
    backgroundColor = UIColor.clear
  }
}
