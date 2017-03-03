final class CarouselElement : UICollectionViewCell {
  @IBOutlet private weak var image: UIImageView!
  @IBOutlet private var dimMask: [UIView]!

  func config(with url: URL, isLastCell: Bool) {
    image.af_setImage(withURL: url, imageTransition: .crossDissolve(kDefaultAnimationDuration))
    dimMask.forEach { $0.isHidden = !isLastCell }
  }
}

@objc(MWMPPHotelCarouselCell)
final class PPHotelCarouselCell: MWMTableViewCell {

  @IBOutlet private weak var collectionView: UICollectionView!
  fileprivate var dataSource: [GalleryItemModel] = []
  fileprivate let kMaximumNumberOfPhotos = 5
  fileprivate var delegate: MWMPlacePageButtonsProtocol?

  func config(with ds: [GalleryItemModel], delegate d: MWMPlacePageButtonsProtocol?) {
    dataSource = ds
    delegate = d
    collectionView.contentOffset = .zero
    collectionView.delegate = self
    collectionView.dataSource = self
    collectionView.register(cellClass: CarouselElement.self)
    collectionView.reloadData()
  }
}

extension PPHotelCarouselCell: UICollectionViewDelegate, UICollectionViewDataSource {
  private var isFullPhotosCarousel: Bool {
    return dataSource.count <= kMaximumNumberOfPhotos
  }

  private func isLastCell(_ indexPath: IndexPath) -> Bool {
    return (indexPath.item == kMaximumNumberOfPhotos - 1) && !isFullPhotosCarousel
  }

  func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
    let cell = collectionView.dequeueReusableCell(withCellClass: CarouselElement.self,
                                                  indexPath: indexPath) as! CarouselElement

    cell.config(with: dataSource[indexPath.item].imageURL, isLastCell: isLastCell(indexPath))
    return cell
  }

  func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
    return isFullPhotosCarousel ? dataSource.count : kMaximumNumberOfPhotos
  }

  func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    guard let d = delegate else { return }

    if isLastCell(indexPath) {
      d.showGalery()
    } else {
      d.showPhoto(at: UInt(indexPath.row))
    }
  }
}
