final class CarPlayMapViewController: MWMViewController {
  private(set) var mapView: EAGLView?
  @IBOutlet var speedInfoView: UIView!
  @IBOutlet var speedLimitContainer: UIView!
  @IBOutlet var speedCamImageView: UIImageView!
  @IBOutlet var speedLimitLabel: UILabel!
  @IBOutlet var currentSpeedView: UIView!
  @IBOutlet var currentSpeedLabel: UILabel!
  private var currentSpeed: Int = 0
  private var speedLimit: Int?
  private var isCameraOnRoute: Bool = false
  private var viewPortState: CPViewPortState = .default
  private var isLeftWheelCar: Bool {
    return self.speedInfoView.frame.origin.x > self.view.frame.midX
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
  }
  
  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    if mapView?.drapeEngineCreated == false {
      mapView?.createDrapeEngine()
    }
    updateVisibleViewPortState(viewPortState)
  }
  
  func addMapView(_ mapView: EAGLView, mapButtonSafeAreaLayoutGuide: UILayoutGuide) {
    mapView.translatesAutoresizingMaskIntoConstraints = false
    removeMapView()
    
    self.mapView = mapView
    mapView.frame = view.bounds
    view.insertSubview(mapView, at: 0)
    mapView.topAnchor.constraint(equalTo: view.topAnchor).isActive = true
    mapView.bottomAnchor.constraint(equalTo: view.bottomAnchor).isActive = true
    mapView.leadingAnchor.constraint(equalTo: view.leadingAnchor).isActive = true
    mapView.trailingAnchor.constraint(equalTo: view.trailingAnchor).isActive = true
    speedInfoView.trailingAnchor.constraint(equalTo: mapButtonSafeAreaLayoutGuide.trailingAnchor).isActive = true
  }
  
  func removeMapView() {
    if let mapView = mapView {
      mapView.removeFromSuperview()
      self.mapView = nil
    }
  }
  
  
  func hideSpeedControl() {
    if !speedInfoView.isHidden {
      speedInfoView.isHidden = true
    }
  }
  
  func showSpeedControl() {
    if speedInfoView.isHidden {
      speedInfoView.isHidden = false
    }
  }
  
  func updateCurrentSpeed(_ speed: Int) {
    self.currentSpeed = speed
    updateSpeedControl()
  }
  
  func updateCameraInfo(isCameraOnRoute: Bool, speedLimit: Int?) {
    self.isCameraOnRoute = isCameraOnRoute
    self.speedLimit = speedLimit
    updateSpeedControl()
  }
  
  private func updateSpeedControl() {
    currentSpeedLabel.text = "\(currentSpeed)"
    if isCameraOnRoute {
      speedLimitContainer.layer.borderColor = UIColor.speedLimitRed().cgColor
      speedLimitContainer.layer.borderWidth = 2.0
      if let speedLimit = speedLimit {
        speedCamImageView.alpha = 0.0
        speedLimitLabel.textColor = UIColor.speedLimitDarkGray()
        speedLimitLabel.text = "\(speedLimit)"
        speedLimitLabel.alpha = 1.0
        currentSpeedLabel.textColor = UIColor.white
        if speedLimit >= currentSpeed {
          currentSpeedView.backgroundColor = UIColor.speedLimitGeen()
        } else {
          currentSpeedView.backgroundColor = UIColor.speedLimitRed()
        }
      } else {
        speedLimitLabel.alpha = 0.0
        speedCamImageView.tintColor = UIColor.speedLimitRed()
        speedCamImageView.alpha = 1.0
        currentSpeedLabel.textColor = UIColor.speedLimitDarkGray()
        currentSpeedView.backgroundColor = UIColor.speedLimitWhite()
      }
    } else {
      speedLimitContainer.layer.borderColor = UIColor.speedLimitLightGray().cgColor
      speedLimitContainer.layer.borderWidth = 2.0
      speedLimitLabel.alpha = 0.0
      speedCamImageView.tintColor = UIColor.speedLimitLightGray()
      speedCamImageView.alpha = 1.0
      currentSpeedLabel.textColor = UIColor.speedLimitDarkGray()
      currentSpeedView.backgroundColor = UIColor.speedLimitWhite()
    }
  }
  
  func updateVisibleViewPortState(_ state: CPViewPortState) {
    viewPortState = state
    switch viewPortState {
    case .default:
      updateVisibleViewPortToDefaultState()
    case .preview:
      updateVisibleViewPortToPreviewState()
    case .navigation:
      updateVisibleViewPortToNavigationState()
    }
  }
  
  private func updateVisibleViewPortToPreviewState() {
    let viewBounds = view.bounds
    let previewWidth = self.view.frame.width * 0.45
    let navigationHeight: CGFloat = 45.0
    var frame = CGRect.zero
    let origin = isLeftWheelCar ? CGPoint(x: previewWidth, y: navigationHeight) : CGPoint(x: 0, y: navigationHeight)
    frame.origin = origin
    frame.size = CGSize(width: viewBounds.width - origin.x,
                        height: viewBounds.height - origin.y)
    FrameworkHelper.setVisibleViewport(frame, scaleFactor: mapView?.contentScaleFactor ?? 1)
  }
  
  private func updateVisibleViewPortToNavigationState() {
    let viewBounds = view.bounds
    let previewWidth = viewBounds.width * 0.45
    let mapControlsWidth = viewBounds.width * 0.15
    let navigationHeight: CGFloat = 45.0
    var frame = CGRect.zero
    let origin = isLeftWheelCar ? CGPoint(x: previewWidth, y: navigationHeight) : CGPoint(x: 0, y: navigationHeight)
    frame.origin = origin
    frame.size = CGSize(width: viewBounds.width - (origin.x + mapControlsWidth),
                        height: viewBounds.height - origin.y)
    FrameworkHelper.setVisibleViewport(frame, scaleFactor: mapView?.contentScaleFactor ?? 1)
  }
  
  private func updateVisibleViewPortToDefaultState() {
    FrameworkHelper.setVisibleViewport(view.bounds, scaleFactor: mapView?.contentScaleFactor ?? 1)
  }
  
  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    ThemeManager.invalidate()
  }

  override func applyTheme() {
    super.applyTheme()
    updateSpeedControl()
  }
}
