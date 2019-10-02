
final class TagCollectionViewCell: UICollectionViewCell {
  
  @IBOutlet private weak var title: UILabel!
  @IBOutlet weak var containerView: UIView!
  
  var color: UIColor? {
    didSet {
      title.textColor = color
      containerView.layer.borderColor = color?.cgColor
    }
  }
  
  override var isSelected: Bool {
    didSet {
      updateSelectedState()
    }
  }
  
  func update(with tag: MWMTag, enabled: Bool) {
    title.text = tag.name
    if enabled {
      color = tag.color
      updateSelectedState()
    } else {
      color = .blackDividers()
    }
  }
  
  func updateSelectedState() {
    containerView.backgroundColor = isSelected ? color : .white()
    title.textColor = isSelected ? .whitePrimaryText() : color
  }
  
  override func prepareForReuse() {
    super.prepareForReuse()
    title.text = nil
    color = nil
  }
}
