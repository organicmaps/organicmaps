final class TagCollectionViewCell: UICollectionViewCell {
  
  @IBOutlet private weak var title: UILabel!
  @IBOutlet weak var containerView: UIView!
  
  var color: UIColor? {
    didSet {
      title.textColor = color
      containerView.layer.borderColor = color?.cgColor
    }
  }
  
  override var isSelected: Bool{
    didSet {
      containerView.backgroundColor = isSelected ? color : .white()
      title.textColor = isSelected ? .whitePrimaryText() : color
    }
  }
  
  func update(with tag: MWMTag) {
    title.text = tag.name
    color = tag.color
  }
  
  override func prepareForReuse() {
    super.prepareForReuse()
    title.text = nil
    color = nil
  }
}
