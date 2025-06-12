
private let kUDDidShowFirstTimeRoutingEducationalHint = "kUDDidShowFirstTimeRoutingEducationalHint"

class BottomTabBarViewController: UIViewController {

  private enum Constants {
    static let blinkingDuration = 1.0
    static let color: (lighter: UIColor, darker: UIColor) = (.red, .red.darker(percent: 0.3))
  }

  var presenter: BottomTabBarPresenterProtocol!
  
  @IBOutlet var searchButton: MWMButton!
  @IBOutlet var helpButton: MWMButton!
  @IBOutlet var bookmarksButton: MWMButton!
  @IBOutlet var trackRecordingButton: BottomTabBarButton!
  @IBOutlet var moreButton: MWMButton!
  @IBOutlet var downloadBadge: UIView!
  @IBOutlet var helpBadge: UIView!
  
  private var avaliableArea = CGRect.zero
  @objc var isHidden: Bool = false {
    didSet {
      updateFrame(animated: true)
    }
  }
  @objc var isApplicationBadgeHidden: Bool = true {
    didSet {
      updateBadge()
    }
  }
  var trackRecordingState: TrackRecordingState = .inactive {
    didSet {
      guard trackRecordingState != oldValue else { return }
      updateTrackRecordingButton()
    }
  }

  private var trackRecordingBlinkTimer: Timer?

  var tabBarView: BottomTabBarView {
    return view as! BottomTabBarView
  }

  @objc static var controller: BottomTabBarViewController? {
    return MWMMapViewControlsManager.manager()?.tabBarController
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    presenter.configure()
  }
  
  override func viewWillAppear(_ animated: Bool) {
    super.viewWillAppear(animated)
    if Settings.isNY() {
      helpButton.setTitle("🎄", for: .normal)
      helpButton.setImage(nil, for: .normal)
    }
    updateBadge()
  }

  static func updateAvailableArea(_ frame: CGRect) {
    BottomTabBarViewController.controller?.updateAvailableArea(frame)
  }
  
  @IBAction func onSearchButtonPressed(_ sender: Any) {
    presenter.onSearchButtonPressed()
  }
  
  @IBAction func onHelpButtonPressed(_ sender: Any) {
    if !helpBadge.isHidden {
      presenter.onHelpButtonPressed(withBadge: true)
      setHelpBadgeShown()
    } else {
      presenter.onHelpButtonPressed(withBadge: false)
    }
  }
  
  @IBAction func onBookmarksButtonPressed(_ sender: Any) {
    presenter.onBookmarksButtonPressed()
  }

  @IBAction func onTrackRecordingButtonPressed(_ sender: Any) {
    presenter.onTrackRecordingButtonPressed()
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
    downloadBadge.isHidden = isApplicationBadgeHidden
    helpBadge.isHidden = !needsToShowHelpBadge()
  }

  private func updateTrackRecordingButton() {
    if trackRecordingState == .active {
      var lighter = false
      trackRecordingBlinkTimer?.invalidate()
      trackRecordingBlinkTimer = Timer.scheduledTimer(withTimeInterval: Constants.blinkingDuration, repeats: true) { [weak self] _ in
        UIView.animate(withDuration: Constants.blinkingDuration, animations: {
          self?.trackRecordingButton.tintColor = lighter ? Constants.color.lighter : Constants.color.darker
          lighter.toggle()
        })
      }
    } else {
      trackRecordingBlinkTimer?.invalidate()
      trackRecordingBlinkTimer = nil
      trackRecordingButton.coloring = .black
    }
  }
}

// MARK: - Help badge
private extension BottomTabBarViewController {
  private func needsToShowHelpBadge() -> Bool {
    !UserDefaults.standard.bool(forKey: kUDDidShowFirstTimeRoutingEducationalHint)
  }
  
  private func setHelpBadgeShown() {
    UserDefaults.standard.set(true, forKey: kUDDidShowFirstTimeRoutingEducationalHint)
  }
}
