import UIKit

final class PhotoViewController: UIViewController {
  let scalingImageView = PhotoScalingImageView()

  let photo: GalleryItemModel

  private(set) lazy var doubleTapGestureRecognizer: UITapGestureRecognizer = {
    let gesture = UITapGestureRecognizer(target: self, action: #selector(handleDoubleTapWithGestureRecognizer(_:)))
    gesture.numberOfTapsRequired = 2
    return gesture
  }()

  init(photo: GalleryItemModel) {
    self.photo = photo
    super.init(nibName: nil, bundle: nil)
  }
  
  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    scalingImageView.delegate = self
    scalingImageView.frame = view.bounds
    scalingImageView.autoresizingMask = [.flexibleWidth, .flexibleHeight]
    view.addSubview(scalingImageView)

    view.addGestureRecognizer(doubleTapGestureRecognizer)

    scalingImageView.photo = photo
  }

  override func viewWillLayoutSubviews() {
    super.viewWillLayoutSubviews()
    scalingImageView.frame = view.bounds
  }

  @objc
  private func handleDoubleTapWithGestureRecognizer(_ recognizer: UITapGestureRecognizer) {
    let pointInView = recognizer.location(in: scalingImageView.imageView)
    var newZoomScale = scalingImageView.maximumZoomScale

    if scalingImageView.zoomScale >= scalingImageView.maximumZoomScale ||
       abs(scalingImageView.zoomScale - scalingImageView.maximumZoomScale) <= 0.01 {
      newZoomScale = scalingImageView.minimumZoomScale
    }

    let scrollViewSize = scalingImageView.bounds.size
    let width = scrollViewSize.width / newZoomScale
    let height = scrollViewSize.height / newZoomScale
    let originX = pointInView.x - (width / 2.0)
    let originY = pointInView.y - (height / 2.0)

    let rectToZoom = CGRect(x: originX, y: originY, width: width, height: height)
    scalingImageView.zoom(to: rectToZoom, animated: true)
  }
}

extension PhotoViewController: UIScrollViewDelegate {
  func viewForZooming(in scrollView: UIScrollView) -> UIView? {
    return scalingImageView.imageView
  }

  func scrollViewWillBeginZooming(_ scrollView: UIScrollView, with view: UIView?) {
    scrollView.panGestureRecognizer.isEnabled = true
  }

  func scrollViewDidEndZooming(_ scrollView: UIScrollView, with view: UIView?, atScale scale: CGFloat) {
    if (scrollView.zoomScale == scrollView.minimumZoomScale) {
      scrollView.panGestureRecognizer.isEnabled = false
    }
  }
}
