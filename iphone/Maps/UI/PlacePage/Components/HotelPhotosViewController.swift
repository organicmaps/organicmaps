final class PhotoCell: UICollectionViewCell {
  @IBOutlet var imageView: UIImageView!
  @IBOutlet var dimView: UIView!

  func config(_ imageUrlString: String, dimmed: Bool = false) {
    guard let imageUrl = URL(string: imageUrlString) else { return }
    imageView.wi_setImage(with: imageUrl)
    dimView.isHidden = !dimmed
  }

  override func prepareForReuse() {
    super.prepareForReuse()
    imageView.wi_cancelImageRequest()
  }
}

protocol HotelPhotosViewControllerDelegate: AnyObject {
  func didSelectItemAt(_ hotelPhotosViewController: HotelPhotosViewController, index: Int, lastItemIndex: Int)
}

final class HotelPhotosViewController: UIViewController {
  @IBOutlet var collectionView: UICollectionView!

  var photos: [HotelPhotoUrl]? {
    didSet {
      collectionView?.reloadData()
    }
  }
  weak var delegate: HotelPhotosViewControllerDelegate?

  func viewForPhoto(_ photo: HotelPhotoUrl) -> UIView? {
    guard let index = photos?.firstIndex(where: {
      photo === $0
    }) else {
      return nil
    }

    let indexPath = IndexPath(item: index, section: 0)
    return collectionView.cellForItem(at: indexPath)
  }
}

extension HotelPhotosViewController: UICollectionViewDataSource {
  func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
    min(photos?.count ?? 0, 5)
  }

  func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
    let photoCell = collectionView.dequeueReusableCell(withReuseIdentifier: "PhotoCell", for: indexPath) as! PhotoCell
    if let photoItem = photos?[indexPath.item] {
      photoCell.config(photoItem.thumbnail)
    }
    return photoCell
  }
}

extension HotelPhotosViewController: UICollectionViewDelegate {
  func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    delegate?.didSelectItemAt(self, index: indexPath.item, lastItemIndex: collectionView.numberOfItems(inSection: 0) - 1)
  }
}
