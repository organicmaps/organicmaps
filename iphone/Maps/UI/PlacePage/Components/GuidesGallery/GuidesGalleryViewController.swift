import UIKit

protocol IGuidesGalleryView: AnyObject {
  func setGalleryItems(_ items: [IGuidesGalleryItemViewModel])
  func setActiveItem(_ index: Int, animated: Bool)
  func updateItem(_ item: IGuidesGalleryItemViewModel, at index: Int)
}

final class GuidesGalleryViewController: UIViewController {
  @IBOutlet private var collectionView: UICollectionView!
  var presenter: IGuidesGalleryPresenter!
  private var galleryItems: [IGuidesGalleryItemViewModel] = []
  private var selectedIndex = 0

  override func viewDidLoad() {
    super.viewDidLoad()

    let layout = RoutesGalleryLayout()
    layout.onScrollToItem = { [weak self] index in
      guard let self = self, self.selectedIndex != index else { return }
      self.presenter.scrollToItemAtIndex(index)
      self.selectedIndex = index
    }
    collectionView.collectionViewLayout = layout
    collectionView.decelerationRate = .fast
    presenter.viewDidLoad()
  }

  private func applyTransform() {
    guard let layout = collectionView.collectionViewLayout as? UICollectionViewFlowLayout else { return }
    let pageWidth = layout.itemSize.width + layout.minimumLineSpacing
    for cell in collectionView.visibleCells {
      let cellX = cell.convert(.zero, to: view).x
      let distance = abs(layout.sectionInset.left - cellX)
      let scale = max(1 - distance / pageWidth, 0)
      cell.transform = CGAffineTransform(translationX: 0, y: -8 * scale)
    }
  }
}

extension GuidesGalleryViewController: UICollectionViewDataSource {
  func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
    galleryItems.count
  }

  func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
    let galleryItem = galleryItems[indexPath.item]
    switch galleryItem {
    case let cityItem as IGuidesGalleryCityItemViewModel:
      let cell = collectionView.dequeueReusableCell(cell: GuidesGalleryCityCell.self, indexPath: indexPath)
      cell.config(cityItem)
      cell.delegate = self
      return cell
    case let outdoorItem as IGuidesGalleryOutdoorItemViewModel:
      let cell = collectionView.dequeueReusableCell(cell: GuidesGalleryOutdoorCell.self, indexPath: indexPath)
      cell.config(outdoorItem)
      cell.delegate = self
      return cell
    default:
      fatalError("Unexpected item type \(galleryItem)")
    }
  }
}

extension GuidesGalleryViewController: UICollectionViewDelegate {
  func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    presenter.selectItemAtIndex(indexPath.item)
  }

  func scrollViewDidScroll(_ scrollView: UIScrollView) {
    applyTransform()
  }
}

extension GuidesGalleryViewController: IGuidesGalleryView {
  func setGalleryItems(_ items: [IGuidesGalleryItemViewModel]) {
    galleryItems = items
    collectionView.reloadData()
  }

  func setActiveItem(_ index: Int, animated: Bool) {
    selectedIndex = index
    collectionView.performBatchUpdates(nil) { [weak self] _ in
      guard let layout = self?.collectionView.collectionViewLayout as? UICollectionViewFlowLayout else { return }
      let itemWidth = layout.itemSize.width + layout.minimumInteritemSpacing
      self?.collectionView.setContentOffset(CGPoint(x: itemWidth * CGFloat(index), y: 0), animated: animated)
      self?.collectionView.layoutIfNeeded()
      self?.applyTransform()
    }
  }

  func updateItem(_ item: IGuidesGalleryItemViewModel, at index: Int) {
    galleryItems[index] = item
    let indexPath = IndexPath(item: index, section: 0)
    guard let cell = collectionView.cellForItem(at: indexPath) as? GuidesGalleryCell else { return }
    cell.config(item)
  }
}

extension GuidesGalleryViewController: GuidesGalleryCellDelegate {
  func onShowButton(_ cell: GuidesGalleryCell) {
    guard let indexPath = collectionView.indexPath(for: cell) else { return }
    presenter.toggleVisibilityAtIndex(indexPath.item)
  }
}

fileprivate final class RoutesGalleryLayout: UICollectionViewFlowLayout {
  typealias OnScrollToItemClosure = (Int) -> Void
  var onScrollToItem: OnScrollToItemClosure?

  override func prepare() {
    super.prepare()

    guard let collectionView = collectionView else { return }
    let availableWidth = collectionView.bounds.width
    let itemWidth = min(availableWidth - 48, 366)
    itemSize = CGSize(width: itemWidth, height: 120)
    sectionInset = UIEdgeInsets(top: 16, left: 16, bottom: 16, right: 16)
    collectionView.contentInset = UIEdgeInsets(top: 0,
                                               left: 0,
                                               bottom: 0,
                                               right: availableWidth - itemWidth - sectionInset.left - sectionInset.right)
    scrollDirection = .horizontal
    minimumLineSpacing = 8
    minimumInteritemSpacing = 8
  }

  override func targetContentOffset(forProposedContentOffset proposedContentOffset: CGPoint,
                                    withScrollingVelocity velocity: CGPoint) -> CGPoint {
    guard let x = collectionView?.contentOffset.x else { return proposedContentOffset }
    let pageWidth = itemSize.width + minimumLineSpacing
    let index = x / pageWidth
    let adjustedIndex: CGFloat
    if velocity.x < 0 {
      adjustedIndex = floor(index)
    } else if velocity.x > 0 {
      adjustedIndex = ceil(index)
    } else {
      adjustedIndex = round(index)
    }
    onScrollToItem?(Int(adjustedIndex))
    return CGPoint(x: adjustedIndex * pageWidth, y: 0)
  }
}
