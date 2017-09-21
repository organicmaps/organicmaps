@objc(MWMGalleryViewController)
final class GalleryViewController: MWMCollectionViewController {
  typealias Cell = GalleryCell
  typealias Model = GalleryModel

  @objc static func instance(model: Model) -> GalleryViewController {
    let vc = GalleryViewController(nibName: toString(self), bundle: nil)
    vc.model = model
    return vc
  }

  private var model: Model!

  private var lastViewSize = CGSize.zero {
    didSet { configLayout() }
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    title = model.title
    collectionView!.register(cellClass: Cell.self)
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    lastViewSize = view.size
  }

  private func configLayout() {
    let minItemsPerRow: CGFloat = 3
    let maxItemWidth: CGFloat = 120
    let ratio: CGFloat = 3.0 / 4.0
    let viewWidth = view.size.width
    let spacing: CGFloat = 4

    let itemsPerRow = floor(max(viewWidth / maxItemWidth, minItemsPerRow))
    let itemWidth = (viewWidth - (itemsPerRow + 1) * spacing) / itemsPerRow
    let itemHeight = itemWidth * ratio

    let layout = collectionView!.collectionViewLayout as! UICollectionViewFlowLayout
    layout.minimumLineSpacing = spacing
    layout.minimumInteritemSpacing = spacing
    layout.itemSize = CGSize(width: itemWidth, height: itemHeight)
  }

  override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
    coordinator.animate(alongsideTransition: { [unowned self] _ in
      self.lastViewSize = size
    })
  }

  // MARK: UICollectionViewDataSource
  override func collectionView(_: UICollectionView, numberOfItemsInSection _: Int) -> Int {
    return model.items.count
  }

  override func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
    let cell = collectionView.dequeueReusableCell(withCellClass: Cell.self,
                                                  indexPath: indexPath) as! Cell
    cell.model = model.items[indexPath.item]
    return cell
  }

  // MARK: UICollectionViewDelegate
  override func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    let currentPhoto = model.items[indexPath.item]
    let cell = collectionView.cellForItem(at: indexPath)
    let photoVC = PhotosViewController(photos: model, initialPhoto: currentPhoto, referenceView: cell)

    photoVC.referenceViewForPhotoWhenDismissingHandler = { [weak self] photo in
      if let index = self?.model.items.index(where: { $0 === photo }) {
        let indexPath = IndexPath(item: index, section: 0)
        return collectionView.cellForItem(at: indexPath)
      }
      return nil
    }
    present(photoVC, animated: true, completion: nil)
  }
}
