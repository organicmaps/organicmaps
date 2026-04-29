final class CarPlayMapViewController: MWMViewController {
  private(set) var mapView: EAGLView?
  @IBOutlet var speedInfoView: UIView!
  @IBOutlet var speedCamLimitContainer: UIView!
  @IBOutlet var speedCamImageView: UIImageView!
  @IBOutlet var speedCamLimitLabel: UILabel!
  @IBOutlet var currentSpeedView: UIView!
  @IBOutlet var currentSpeedLabel: UILabel!
  private var currentSpeedMps: Double = 0.0
  private var speedLimitMps: Double?
  private var speedCamLimitMps: Double?
  private var isCameraOnRoute: Bool = false
  private var viewPortState: CPViewPortState = .default
  private var isSpeedCamBlinking: Bool = false

  override func viewDidLoad() {
    super.viewDidLoad()
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    if mapView?.drapeEngineCreated == false, !MapsAppDelegate.isTestsEnvironment() {
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

    speedCamLimitContainer.layer.borderWidth = 2.0
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

  func updateCurrentSpeed(_ speedMps: Double, speedLimitMps: Double?) {
    currentSpeedMps = speedMps
    self.speedLimitMps = speedLimitMps
    updateSpeedControl()
  }

  func updateCameraInfo(isCameraOnRoute: Bool, speedLimitMps: Double?) {
    self.isCameraOnRoute = isCameraOnRoute
    speedCamLimitMps = speedLimitMps
    updateSpeedControl()
  }

  private func BlinkSpeedCamLimit(blink: Bool) {
    if blink {
      if !isSpeedCamBlinking {
        speedCamLimitLabel.alpha = 0
        speedCamImageView.alpha = 1
        UIView.animate(withDuration: 0.5,
                       delay: 0.0,
                       options: [.repeat, .autoreverse, .curveEaseOut],
                       animations: { self.speedCamImageView.alpha = 0; self.speedCamLimitLabel.alpha = 1 })
        isSpeedCamBlinking = true
      }
    } else {
      if isSpeedCamBlinking {
        speedCamLimitLabel.layer.removeAllAnimations()
        speedCamImageView.layer.removeAllAnimations()
        isSpeedCamBlinking = false
      }
    }
  }

  private func updateSpeedControl() {
    let speedMeasure = Measure(asSpeed: currentSpeedMps)
    currentSpeedLabel.text = speedMeasure.valueAsString

    if isCameraOnRoute {
      speedCamLimitContainer.layer.borderColor = UIColor.speedLimitRed.cgColor
      speedCamImageView.tintColor = .speedLimitRed

      // self.speedCamLimitMps comes from SpeedCamManager and is based on
      // the nearest speed camera info when it is close enough.
      // If it's unknown self.speedLimitMps is used, which is based on current road speed limit.
      if let speedCamLimitMps = (speedCamLimitMps ?? speedLimitMps) {
        BlinkSpeedCamLimit(blink: true)
        let speedCamLimitMeasure = Measure(asSpeed: speedCamLimitMps)
        speedCamLimitLabel.text = speedCamLimitMeasure.valueAsString
        speedCamLimitLabel.textColor = .speedLimitDarkGray

        currentSpeedLabel.textColor = UIColor.whitePrimary
        if speedCamLimitMps >= currentSpeedMps {
          currentSpeedView.backgroundColor = .speedLimitGreen
        } else {
          currentSpeedView.backgroundColor = .speedLimitRed
        }
      } else {
        BlinkSpeedCamLimit(blink: false)
        speedCamLimitLabel.alpha = 0.0
        speedCamImageView.tintColor = .speedLimitRed
        speedCamImageView.alpha = 1.0

        currentSpeedLabel.textColor = .speedLimitDarkGray
        currentSpeedView.backgroundColor = .speedLimitWhite
      }
    } else { // !isCameraOnRoute
      BlinkSpeedCamLimit(blink: false)
      currentSpeedLabel.textColor = .speedLimitDarkGray
      if let speedLimitMps = speedLimitMps {
        speedCamImageView.alpha = 0.0
        let speedLimitMeasure = Measure(asSpeed: speedLimitMps)
        speedCamLimitLabel.textColor = .speedLimitDarkGray
        // speedLimitMps == 0 means unlimited speed.
        if speedLimitMeasure.value == 0 {
          speedCamLimitLabel.text = "🚀" // "∞"
        } else {
          speedCamLimitLabel.text = speedLimitMeasure.valueAsString
        }
        speedCamLimitLabel.alpha = 1.0
        speedCamLimitContainer.layer.borderColor = UIColor.speedLimitRed.cgColor
        if currentSpeedMps > speedLimitMps {
          currentSpeedLabel.textColor = .speedLimitRed
        }
      } else {
        speedCamImageView.tintColor = .speedLimitLightGray
        speedCamImageView.alpha = 1.0
        speedCamLimitLabel.alpha = 0.0
        speedCamLimitContainer.layer.borderColor = UIColor.speedLimitLightGray.cgColor
      }
      currentSpeedView.backgroundColor = .speedLimitWhite
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
    updateVisibleViewPort(frame: view.frame.inset(by: view.safeAreaInsets))
  }

  private func updateVisibleViewPortToNavigationState() {
    updateVisibleViewPort(frame: view.frame.inset(by: view.safeAreaInsets))
  }

  private func updateVisibleViewPortToDefaultState() {
    updateVisibleViewPort(frame: view.bounds)
  }

  private func updateVisibleViewPort(frame: CGRect) {
    guard CarPlayService.shared.isCarplayActivated else { return }
    FrameworkHelper.setVisibleViewport(frame, scaleFactor: mapView?.contentScaleFactor ?? 1)
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    // Triggers the map style updating when CarPlay's 'Appearance' setting is changed.
    ThemeManager.invalidate()
  }

  override func applyTheme() {
    super.applyTheme()
    updateSpeedControl()
  }
}
