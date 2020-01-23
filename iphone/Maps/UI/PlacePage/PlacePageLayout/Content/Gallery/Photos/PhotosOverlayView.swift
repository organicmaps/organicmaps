final class PhotosOverlayView: UIView {
  private var navigationBar: UINavigationBar!
  private var navigationItem: UINavigationItem!

  weak var photosViewController: PhotosViewController?

  var photo: HotelPhotoUrl? {
    didSet {
      guard let photo = photo else {
        navigationItem.title = nil
        return
      }
      guard let photosViewController = photosViewController else { return }
      if let index = photosViewController.photos.firstIndex(where: { $0 === photo }) {
        navigationItem.title = "\(index + 1) / \(photosViewController.photos.count)"
      }
    }
  }

  override init(frame: CGRect) {
    super.init(frame: frame)
    setupNavigationBar()
  }

  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  @objc
  private func closeButtonTapped(_: UIBarButtonItem) {
    photosViewController?.dismiss(animated: true, completion: nil)
  }

  override func hitTest(_ point: CGPoint, with event: UIEvent?) -> UIView? {
    if let hitView = super.hitTest(point, with: event), hitView != self {
      return hitView
    }
    return nil
  }

  private func setupNavigationBar() {
    navigationBar = UINavigationBar()
    navigationBar.translatesAutoresizingMaskIntoConstraints = false
    navigationBar.backgroundColor = UIColor.clear
    navigationBar.barTintColor = nil
    navigationBar.isTranslucent = true
    navigationBar.setBackgroundImage(UIImage(), for: .default)

    navigationItem = UINavigationItem(title: "")
    navigationBar.items = [navigationItem]
    addSubview(navigationBar)

    navigationBar.topAnchor.constraint(equalTo: self.safeAreaLayoutGuide.topAnchor).isActive = true
    navigationBar.widthAnchor.constraint(equalTo: self.widthAnchor).isActive = true
    navigationBar.centerXAnchor.constraint(equalTo: self.centerXAnchor).isActive = true

    navigationItem.leftBarButtonItem = UIBarButtonItem(image: #imageLiteral(resourceName: "ic_nav_bar_back"), style: .plain, target: self, action: #selector(closeButtonTapped(_:)))
  }

  func setHidden(_ hidden: Bool, animated: Bool, animation: @escaping (() -> Void)) {
    guard isHidden != hidden else { return }
    guard animated else {
      isHidden = hidden
      animation()
      return
    }
    isHidden = false
    alpha = hidden ? 1.0 : 0.0

    UIView.animate(withDuration: kDefaultAnimationDuration,
                   delay: 0.0,
                   options: [.allowAnimatedContent, .allowUserInteraction],
                   animations: { [weak self] in
                     self?.alpha = hidden ? 0.0 : 1.0
                     animation()
                   },
                   completion: { [weak self] _ in
                     self?.alpha = 1.0
                     self?.isHidden = hidden
    })
  }
}
