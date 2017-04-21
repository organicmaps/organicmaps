@objc (MWMFilterPriceCategoryCell)
final class FilterPriceCategoryCell: UITableViewCell {
  
  @IBOutlet weak var one: UIButton!
  @IBOutlet weak var two: UIButton!
  @IBOutlet weak var three: UIButton!

  @IBAction private func tap(sender: UIButton!) {
    sender.isSelected = !sender.isSelected;
  }
}
