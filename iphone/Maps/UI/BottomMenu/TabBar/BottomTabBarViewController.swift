protocol BottomTabBarViewProtocol: class {
  var presenter: BottomTabBarPresenterProtocol! { get set }
  var isHidden: Bool { get }
  var isLayersBadgeHidden: Bool { get set }
  var isApplicationBadgeHidden: Bool { get set }
}

class BottomTabBarViewController: UIViewController {
  var presenter: BottomTabBarPresenterProtocol!
  
  @IBOutlet var searchButton: MWMButton!
  @IBOutlet var routeButton: MWMButton!
  @IBOutlet var discoveryButton: MWMButton!
  @IBOutlet var bookmarksButton: MWMButton!
  @IBOutlet var moreButton: MWMButton!
  @IBOutlet var downloadBadge: UIView!
  
  private var avaliableArea = CGRect.zero
  @objc var isHidden: Bool = false {
    didSet {
      updateFrame(animated: true)
    }
  }
  var isLayersBadgeHidden: Bool = true {
    didSet {
      updateBadge()
    }
  }
  @objc var isApplicationBadgeHidden: Bool = true {
    didSet {
      updateBadge()
    }
  }
  var tabBarView: BottomTabBarView {
    return view as! BottomTabBarView
  }
  @objc static var controller: BottomTabBarViewController? {
    return MWMMapViewControlsManager.manager()?.tabBarController
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    presenter.configure()
    updateBadge()

    MWMSearchManager.add(self)
    MWMNavigationDashboardManager.add(self)
  }
  
  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)
  }
  
  deinit {
    MWMSearchManager.remove(self)
    MWMNavigationDashboardManager.remove(self)
  }
  
  static func updateAvailableArea(_ frame: CGRect) {
    BottomTabBarViewController.controller?.updateAvailableArea(frame)
  }
  
  @IBAction func onSearchButtonPressed(_ sender: Any) {
    presenter.onSearchButtonPressed()
  }
  
  @IBAction func onPoint2PointButtonPressed(_ sender: Any) {
    presenter.onPoint2PointButtonPressed()
  }
  
  @IBAction func onDiscoveryButtonPressed(_ sender: Any) {
    presenter.onDiscoveryButtonPressed()
  }
  
  @IBAction func onBookmarksButtonPressed(_ sender: Any) {
    presenter.onBookmarksButtonPressed()
  }
  
  @IBAction func onMenuButtonPressed(_ sender: Any) {
    presenter.onMenuButtonPressed()
  }
  
  
  private func updateAvailableArea(_ frame:CGRect) {
    avaliableArea = frame
    updateFrame(animated: false)
    self.view.layoutIfNeeded()
  }
  
  private func updateFrame(animated: Bool) {
    if avaliableArea == .zero {
      return
    }
    let newFrame = CGRect(x: avaliableArea.minX,
                          y: isHidden ? avaliableArea.minY + avaliableArea.height : avaliableArea.minY,
                          width: avaliableArea.width,
                          height: avaliableArea.height)
    let alpha:CGFloat = isHidden ? 0 : 1
    if animated {
      UIView.animate(withDuration: kDefaultAnimationDuration,
                     delay: 0,
                     options: [.beginFromCurrentState],
                     animations: {
                      self.view.frame = newFrame
                      self.view.alpha = alpha
      }, completion: nil)
    } else {
      self.view.frame = newFrame
      self.view.alpha = alpha
    }
  }
  
  private func updateBadge() {
    downloadBadge.isHidden = isApplicationBadgeHidden && isLayersBadgeHidden
  }
}

extension BottomTabBarViewController: BottomTabBarViewProtocol {
  
}

// MARK: - MWMNavigationDashboardObserver

extension BottomTabBarViewController: MWMNavigationDashboardObserver {
  func onNavigationDashboardStateChanged() {
    let state = MWMNavigationDashboardManager.shared().state
    self.isHidden = state != .hidden;
  }
}

// MARK: - MWMSearchManagerObserver

extension BottomTabBarViewController: MWMSearchManagerObserver {
  func onSearchManagerStateChanged() {
    let state = MWMSearchManager.manager().state;
    self.searchButton.isSelected = state != .hidden
  }
}
