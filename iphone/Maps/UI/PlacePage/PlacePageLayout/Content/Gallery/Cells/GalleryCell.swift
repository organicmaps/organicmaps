import AlamofireImage

final class GalleryCell: UICollectionViewCell {
  typealias Model = GalleryItemModel

  @IBOutlet weak var imageView: UIImageView!

  var model: Model! {
    didSet {
      imageView.af_setImage(withURL: model.previewURL,
                            placeholderImage: #imageLiteral(resourceName: "ic_placeholder"))
    }
  }

  override func prepareForReuse() {
    imageView.af_cancelImageRequest()
  }
}
