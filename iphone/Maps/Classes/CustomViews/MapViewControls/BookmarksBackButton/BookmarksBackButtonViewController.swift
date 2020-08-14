class BookmarksBackButtonViewController: UIViewController {
  private let kTopOffset: CGFloat = 6
  private var topOffset: NSLayoutConstraint?
  private var leftOffset: NSLayoutConstraint?
  private var availableArea = CGRect.zero
  private var mapViewController = MapViewController.shared()

  @objc var hidden: Bool = true {
    didSet {
      refreshLayout()
    }
  }

  static var controller: BookmarksBackButtonViewController = {
    guard let bookmarksBackButton = MWMMapViewControlsManager.manager()?.bookmarksBackButton else {
      fatalError()
    }
    return bookmarksBackButton
  }()

  override func viewDidLoad() {
    super.viewDidLoad()
    refreshLayout(false)
  }

  @objc func configLayout() {
    guard let superview = view.superview else {
      fatalError()
    }

    topOffset = view.topAnchor.constraint(equalTo: superview.topAnchor, constant: kTopOffset)
    topOffset?.isActive = true
    leftOffset = view.leadingAnchor.constraint(equalTo: superview.leadingAnchor, constant: kViewControlsOffsetToBounds)
    leftOffset?.isActive = true
  }

  func refreshLayout(_ animated: Bool = true) {
    DispatchQueue.main.async {
      let availableArea = self.availableArea
      let leftOffset = self.hidden ? -self.view.width : availableArea.origin.x + kViewControlsOffsetToBounds
      let animations = {
        self.topOffset?.constant = availableArea.origin.y + self.kTopOffset
        self.leftOffset?.constant = leftOffset
        self.view.alpha = self.hidden ? 0 : 1
      }
      if animated {
        self.view.superview?.animateConstraints(animations: animations)
      } else {
        animations()
      }
    }
  }

  class func updateAvailableArea(_ frame: CGRect) {
    let controller = BookmarksBackButtonViewController.controller
    if controller.availableArea.equalTo(frame) {
      return
    }
    controller.availableArea = frame
    controller.refreshLayout()
  }

  @IBAction func buttonTouchUpInside(_ sender: Any) {
    MapViewController.shared()?.bookmarksCoordinator.state = .opened
  }
}
