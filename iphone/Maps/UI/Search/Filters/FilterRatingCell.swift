@objc (MWMFilterRatingCell)
final class FilterRatingCell: UITableViewCell {
  
  @IBOutlet private var ratingButtons: [UIButton]!
  @IBOutlet weak var any: UIButton!
  @IBOutlet weak var good: UIButton!
  @IBOutlet weak var veryGood: UIButton!
  @IBOutlet weak var excellent: UIButton!

  @IBAction private func tap(sender: UIButton!) {
    guard !sender.isSelected else { return }

    ratingButtons.forEach { $0.isSelected = false }
    sender.isSelected = true
  }
}
