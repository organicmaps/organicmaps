import UIKit

final class PhotoViewController: UIViewController {
  let scalingView = PhotoScalingView()

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

  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    scalingView.delegate = self
    scalingView.frame = view.bounds
    scalingView.autoresizingMask = [.flexibleWidth, .flexibleHeight]
    view.addSubview(scalingView)

    view.addGestureRecognizer(doubleTapGestureRecognizer)

    scalingView.photo = photo
  }

  override func viewWillLayoutSubviews() {
    super.viewWillLayoutSubviews()
    scalingView.frame = view.bounds
  }

  @objc
  private func handleDoubleTapWithGestureRecognizer(_ recognizer: UITapGestureRecognizer) {
    let pointInView = recognizer.location(in: scalingView.imageView)
    var newZoomScale = scalingView.maximumZoomScale

    if scalingView.zoomScale >= scalingView.maximumZoomScale ||
      abs(scalingView.zoomScale - scalingView.maximumZoomScale) <= 0.01 {
      newZoomScale = scalingView.minimumZoomScale
    }

    let scrollViewSize = scalingView.bounds.size
    let width = scrollViewSize.width / newZoomScale
    let height = scrollViewSize.height / newZoomScale
    let originX = pointInView.x - (width / 2.0)
    let originY = pointInView.y - (height / 2.0)

    let rectToZoom = CGRect(x: originX, y: originY, width: width, height: height)
    scalingView.zoom(to: rectToZoom, animated: true)
  }
}

extension PhotoViewController: UIScrollViewDelegate {
  func viewForZooming(in _: UIScrollView) -> UIView? {
    return scalingView.imageView
  }

  func scrollViewWillBeginZooming(_ scrollView: UIScrollView, with _: UIView?) {
    scrollView.panGestureRecognizer.isEnabled = true
  }

  func scrollViewDidEndZooming(_ scrollView: UIScrollView, with _: UIView?, atScale _: CGFloat) {
    if scrollView.zoomScale == scrollView.minimumZoomScale {
      scrollView.panGestureRecognizer.isEnabled = false
    }
  }
}
