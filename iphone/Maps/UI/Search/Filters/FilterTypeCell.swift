final class TypeCellFlowLayout: UICollectionViewFlowLayout {
  
  override func layoutAttributesForElements(in rect: CGRect) -> [UICollectionViewLayoutAttributes]? {
    let attributes = super.layoutAttributesForElements(in: rect)

    var leftMargin = sectionInset.left
    var maxY: CGFloat = 0
    attributes?.forEach { layoutAttribute in
      if layoutAttribute.frame.origin.y >= maxY {
        leftMargin = sectionInset.left
      }

      layoutAttribute.frame.origin.x = leftMargin
      leftMargin += layoutAttribute.frame.width + minimumInteritemSpacing
      maxY = max(layoutAttribute.frame.maxY , maxY)
    }
    return attributes
  }
}

@objc(MWMFilterTypeCell)
final class FilterTypeCell: UICollectionViewCell {
  
  @IBOutlet weak var tagName: UILabel!

  override var isSelected: Bool {
    didSet {
      backgroundColor = isSelected ? UIColor.linkBlue() : UIColor.pressBackground()
      tagName.textColor = isSelected ? UIColor.white() : UIColor.blackPrimaryText()
    }
  }

  override var isHighlighted: Bool {
    didSet {
      backgroundColor = isHighlighted ? UIColor.linkBlueHighlighted() : UIColor.pressBackground()
      tagName.textColor = isHighlighted ? UIColor.white() : UIColor.blackPrimaryText()
    }
  }
}
