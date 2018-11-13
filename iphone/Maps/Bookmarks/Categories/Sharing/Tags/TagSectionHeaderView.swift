final class TagSectionHeaderView: UICollectionReusableView {
  
  @IBOutlet weak var title: UILabel!
  
  func update(with group: MWMTagGroup) {
    title.text = group.name.uppercased()
  }
}
