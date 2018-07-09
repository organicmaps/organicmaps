import AlamofireImage

@objc(MWMGalleryItemViewController)
final class GalleryItemViewController: MWMViewController {
  typealias Model = GalleryItemModel

  static func instance(model: Model) -> GalleryItemViewController {
    let vc = GalleryItemViewController(nibName: toString(self), bundle: nil)
    vc.model = model
    return vc
  }

  private var model: Model!

  @IBOutlet private weak var scrollView: UIScrollView!
  fileprivate var imageView: UIImageView!

  override var hasNavigationBar: Bool {
    return false
  }

  override var preferredStatusBarStyle: UIStatusBarStyle {
    return .lightContent
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    imageView = UIImageView(frame: scrollView.bounds)
    imageView.contentMode = .scaleAspectFit
    scrollView.addSubview(imageView)
    imageView.af_setImage(withURL: model.imageURL,
                          imageTransition: .crossDissolve(kDefaultAnimationDuration))
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    imageView.frame = scrollView.bounds
  }

  override func viewWillTransition(to size: CGSize, with coordinator: UIViewControllerTransitionCoordinator) {
    coordinator.animate(alongsideTransition: { [unowned self] _ in
      self.imageView.frame = CGRect(origin: CGPoint.zero, size: size)
    })
  }

  @IBAction func backAction() {
    _ = navigationController?.popViewController(animated: true)
  }
}

extension GalleryItemViewController: UIScrollViewDelegate {
  func viewForZooming(in _: UIScrollView) -> UIView? {
    return imageView
  }
}
