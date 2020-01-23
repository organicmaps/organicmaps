final class GalleryCell: UICollectionViewCell {
  @IBOutlet weak var imageView: UIImageView!

  var photoUrl: HotelPhotoUrl! {
    didSet {
      guard let url = URL(string: photoUrl.original) else { return }
      imageView.wi_setImage(with: url, transitionDuration: kDefaultAnimationDuration)
    }
  }

  override func prepareForReuse() {
    super.prepareForReuse()
    imageView.wi_cancelImageRequest()
    imageView.image = nil;
  }
}
