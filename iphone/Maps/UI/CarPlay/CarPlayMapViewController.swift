final class CarPlayMapViewController: MWMViewController {
  private(set) var mapView: EAGLView?
  private var speedInfoTrailingConstraint: NSLayoutConstraint?
  @IBOutlet private var speedInfoView: CarPlaySpeedInfoView!
  private var viewPortState: CPViewPortState = .default

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
    NSLayoutConstraint.activate([
      mapView.topAnchor.constraint(equalTo: view.topAnchor),
      mapView.bottomAnchor.constraint(equalTo: view.bottomAnchor),
      mapView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      mapView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
    ])
    speedInfoTrailingConstraint = speedInfoView.trailingAnchor.constraint(equalTo: mapButtonSafeAreaLayoutGuide.trailingAnchor)
    speedInfoTrailingConstraint?.isActive = true
  }

  func removeMapView() {
    speedInfoTrailingConstraint?.isActive = false
    speedInfoTrailingConstraint = nil
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
    speedInfoView.updateCurrentSpeed(speedMps, speedLimitMps: speedLimitMps)
  }

  func updateCameraInfo(isCameraOnRoute: Bool, speedLimitMps: Double?) {
    speedInfoView.updateCameraInfo(isCameraOnRoute: isCameraOnRoute, speedLimitMps: speedLimitMps)
  }

  func updateVisibleViewPortState(_ state: CPViewPortState) {
    viewPortState = state
    switch viewPortState {
    case .default:
      updateVisibleViewPort(frame: view.bounds)
    case .preview, .navigation:
      updateVisibleViewPort(frame: view.bounds.inset(by: view.safeAreaInsets))
    }
  }

  private func updateVisibleViewPort(frame: CGRect) {
    guard CarPlayService.shared.isCarplayActivated, let mapView else { return }
    FrameworkHelper.setVisibleViewport(frame, scaleFactor: mapView.contentScaleFactor)
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    // Triggers the map style updating when CarPlay's 'Appearance' setting is changed.
    ThemeManager.invalidate()
  }

  override func applyTheme() {
    super.applyTheme()
    speedInfoView.updateAppearance()
  }
}
