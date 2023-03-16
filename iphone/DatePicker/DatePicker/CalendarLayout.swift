import UIKit

final class CalendarLayout: UICollectionViewFlowLayout {
  override func prepare() {
    super.prepare()

    guard let collectionView = collectionView else { return }
    let availableWidth = collectionView.bounds.width
    minimumLineSpacing = 0
    minimumInteritemSpacing = 0
    let itemWidth = floor(availableWidth / 7)
    let spaceLeft = availableWidth - itemWidth * 7
    sectionInset = UIEdgeInsets(top: 0, left: spaceLeft / 2, bottom: 0, right: spaceLeft / 2)
    itemSize = CGSize(width: itemWidth, height: itemWidth)
    headerReferenceSize = CGSize(width: availableWidth, height: 80)
    sectionHeadersPinToVisibleBounds = true
  }

  override func shouldInvalidateLayout(forBoundsChange newBounds: CGRect) -> Bool {
    true
  }
}
