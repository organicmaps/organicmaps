final class PhotoScalingView: UIScrollView {
  private(set) lazy var imageView: UIImageView = {
    let imageView = UIImageView(frame: self.bounds)
    self.addSubview(imageView)
    return imageView
  }()

  var photo: HotelPhotoUrl? {
    didSet {
      updateImage(photo)
    }
  }

  override var frame: CGRect {
    get { return super.frame }
    set {
      super.frame = newValue
      updateZoomScale()
      centerScrollViewContents()
    }
  }

  override init(frame: CGRect) {
    super.init(frame: frame)
    setupImageScrollView()
    updateZoomScale()
  }

  required init?(coder aDecoder: NSCoder) {
    super.init(coder: aDecoder)
    setupImageScrollView()
    updateZoomScale()
  }

  override func didAddSubview(_ subview: UIView) {
    super.didAddSubview(subview)
    centerScrollViewContents()
  }

  private func setupImageScrollView() {
    showsVerticalScrollIndicator = false
    showsHorizontalScrollIndicator = false
    bouncesZoom = true
    decelerationRate = .fast
  }

  private func centerScrollViewContents() {
    let horizontalInset: CGFloat = contentSize.width < bounds.width ? (bounds.width - contentSize.width) * 0.5 : 0
    let verticalInset: CGFloat = contentSize.height < bounds.height ? (bounds.height - contentSize.height) * 0.5 : 0
    contentInset = UIEdgeInsets(top: verticalInset, left: horizontalInset, bottom: verticalInset, right: horizontalInset)
  }

  private func updateImage(_ photo: HotelPhotoUrl?) {
    guard let photo = photo else { return }
    imageView.transform = CGAffineTransform.identity
    guard let url = URL(string: photo.original) else { return }
    imageView.wi_setImage(with: url,
                          transitionDuration: kDefaultAnimationDuration) { [weak self] (image, error) in
      guard let s = self else { return }
      s.contentSize = image?.size ?? CGSize.zero
      s.imageView.frame = CGRect(origin: CGPoint.zero, size: s.contentSize)
      s.updateZoomScale()
      s.centerScrollViewContents()
    }
  }

  private func updateZoomScale() {
    guard let image = imageView.image else { return }
    let selfSize = bounds.size
    let imageSize = image.size
    let wScale = selfSize.width / imageSize.width
    let hScale = selfSize.height / imageSize.height
    minimumZoomScale = min(wScale, hScale)
    maximumZoomScale = max(max(wScale, hScale), maximumZoomScale)
    zoomScale = minimumZoomScale
    panGestureRecognizer.isEnabled = false
  }
}
