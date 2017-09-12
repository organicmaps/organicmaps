import UIKit

final class PhotosOverlayView: UIView {
  private var navigationBar: UINavigationBar!
  private var navigationItem: UINavigationItem!

  weak var photosViewController: PhotosViewController?

  var photo: GalleryItemModel? {
    didSet {
      guard let photo = photo else {
        navigationItem.title = nil
        return
      }
      guard let photosViewController = photosViewController else { return }
      if let index = photosViewController.photos.items.index(where: { $0 === photo }) {
        navigationItem.title = "\(index + 1) / \(photosViewController.photos.items.count)"
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
    navigationBar.shadowImage = UIImage()
    navigationBar.setBackgroundImage(UIImage(), for: .default)

    navigationItem = UINavigationItem(title: "")
    navigationBar.items = [navigationItem]
    addSubview(navigationBar)

    let topConstraint = NSLayoutConstraint(item: navigationBar, attribute: .top, relatedBy: .equal, toItem: self, attribute: .top, multiplier: 1.0, constant: statusBarHeight())
    let widthConstraint = NSLayoutConstraint(item: navigationBar, attribute: .width, relatedBy: .equal, toItem: self, attribute: .width, multiplier: 1.0, constant: 0.0)
    let horizontalPositionConstraint = NSLayoutConstraint(item: navigationBar, attribute: .centerX, relatedBy: .equal, toItem: self, attribute: .centerX, multiplier: 1.0, constant: 0.0)
    addConstraints([topConstraint, widthConstraint, horizontalPositionConstraint])

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
