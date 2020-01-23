final class GalleryViewController: MWMCollectionViewController {
  static func instance(photos: [HotelPhotoUrl]) -> GalleryViewController {
    let vc = GalleryViewController(nibName: toString(self), bundle: nil)
    vc.photos = photos
    return vc
  }

  private var photos: [HotelPhotoUrl]!

  private var lastViewSize = CGSize.zero {
    didSet { configLayout() }
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    collectionView!.register(cellClass: GalleryCell.self)
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
    layout.sectionInsetReference = .fromSafeArea
  }

  override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
    coordinator.animate(alongsideTransition: { [unowned self] _ in
      self.lastViewSize = size
    })
  }

  // MARK: UICollectionViewDataSource
  override func collectionView(_: UICollectionView, numberOfItemsInSection _: Int) -> Int {
    return photos.count
  }

  override func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
    let cell = collectionView.dequeueReusableCell(withCellClass: GalleryCell.self, indexPath: indexPath) as! GalleryCell
    cell.photoUrl = photos[indexPath.item]
    return cell
  }

  // MARK: UICollectionViewDelegate
  override func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    let currentPhoto = photos[indexPath.item]
    let cell = collectionView.cellForItem(at: indexPath)
    let photoVC = PhotosViewController(photos: photos, initialPhoto: currentPhoto, referenceView: cell)

    photoVC.referenceViewForPhotoWhenDismissingHandler = { [weak self] photo in
      guard let index = self?.photos.firstIndex(where: { $0 === photo }) else { return nil }
      let indexPath = IndexPath(item: index, section: 0)
      return collectionView.cellForItem(at: indexPath)
    }
    present(photoVC, animated: true, completion: nil)
  }
}
