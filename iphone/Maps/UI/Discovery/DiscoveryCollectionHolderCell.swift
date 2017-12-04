private class DiscoveryItemLayout: UICollectionViewFlowLayout {
  override init() {
    super.init()
    scrollDirection = .horizontal
    minimumInteritemSpacing = 8
    var inset = UIEdgeInsets()
    inset.left = 8
    inset.right = 16
    sectionInset = inset
  }

  convenience init(size: CGSize) {
    self.init()
    itemSize = size
    estimatedItemSize = size
  }

  required init?(coder aDecoder: NSCoder) {
    super.init(coder: aDecoder)
  }
}

@objc(MWMDiscoveryCollectionHolderCell)
final class DiscoveryCollectionHolderCell: UITableViewCell {
  private enum Size {
    static let search = CGSize(width: 160.0, height: 128.0)
    static let viator = CGSize(width: 160.0, height: 218.0)
    static let localExpert = CGSize(width: 160.0, height: 196.0)
  }

  @IBOutlet private weak var cellHeight: NSLayoutConstraint!
  @IBOutlet private(set) weak var collectionView: UICollectionView!

  @objc func configSearchLayout() {
    let size = Size.search;
    cellHeight.constant = size.height;
    setNeedsLayout()
    collectionView.collectionViewLayout = DiscoveryItemLayout(size: Size.search)
    collectionView.register(cellClass: DiscoverySearchCell.self)
  }

  @objc func configViatorLayout() {
    let size = Size.viator;
    cellHeight.constant = size.height;
    setNeedsLayout()
    collectionView.collectionViewLayout = DiscoveryItemLayout(size: Size.viator)
    collectionView.register(cellClass: ViatorElement.self)
  }

  @objc func configLocalExpertsLayout() {
    // TODO: config local expert layout and register cell.
  }
}
