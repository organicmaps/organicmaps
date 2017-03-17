import AlamofireImage

final class GalleryCell: UICollectionViewCell {
  typealias Model = GalleryItemModel

  @IBOutlet weak var imageView: UIImageView!

  var model: Model! {
    didSet {
      imageView.af_setImage(withURL: model.imageURL, imageTransition: .crossDissolve(kDefaultAnimationDuration))
    }
  }

  override func prepareForReuse() {
    imageView.af_cancelImageRequest()
  }
}
