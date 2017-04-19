@objc(MWMFilterCollectionHolderCell)
final class FilterCollectionHolderCell: MWMTableViewCell {

  @IBOutlet weak var collectionView: UICollectionView!
  @IBOutlet private weak var collectionViewHeight: NSLayoutConstraint!

  private func layout() {
    collectionView.setNeedsLayout()
    collectionView.layoutIfNeeded()
    collectionViewHeight.constant = collectionView.contentSize.height
  }

  func config() {
    layout()
    collectionView.allowsMultipleSelection = true;
    isSeparatorHidden = true
    backgroundColor = UIColor.pressBackground()
  }
  
  override func layoutSubviews() {
    super.layoutSubviews()
    layout()
  }
}
