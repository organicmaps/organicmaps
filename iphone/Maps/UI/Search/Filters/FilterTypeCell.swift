@objc(MWMFilterTypeCell)
final class FilterTypeCell: UICollectionViewCell {

  @IBOutlet weak var tagName: UILabel!

  override var isSelected: Bool {
    didSet {
      backgroundColor = isSelected ? UIColor.linkBlue() : UIColor.white()
      tagName.textColor = isSelected ? UIColor.white() : UIColor.blackPrimaryText()
    }
  }

  override var isHighlighted: Bool {
    didSet {
      backgroundColor = isHighlighted ? UIColor.linkBlueHighlighted() : UIColor.white()
      tagName.textColor = isHighlighted ? UIColor.white() : UIColor.blackPrimaryText()
    }
  }
}
