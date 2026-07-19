import UIKit

final class CarPlayDashboardMapViewController: MWMViewController {
  private(set) var mapView: EAGLView?
  private let placeholderView = UIControl()
  private let placeholderDescriptionLabel = UILabel()
  private let placeholderActionView = UIView()
  private let placeholderActionLabel = UILabel()
  private var isPhoneModePlaceholderVisible = false
  private let speedInfoView: CarPlaySpeedInfoView = {
    let view = CarPlaySpeedInfoView()
    view.isHidden = true
    return view
  }()

  override func viewDidLoad() {
    super.viewDidLoad()

    setupPlaceholderView()
    speedInfoView.translatesAutoresizingMaskIntoConstraints = false
    view.addSubview(speedInfoView)
    NSLayoutConstraint.activate([
      speedInfoView.topAnchor.constraint(equalTo: view.safeAreaLayoutGuide.topAnchor, constant: 14),
      speedInfoView.trailingAnchor.constraint(equalTo: view.safeAreaLayoutGuide.trailingAnchor, constant: -14),
    ])
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    if mapView?.drapeEngineCreated == false, !MapsAppDelegate.isTestsEnvironment() {
      mapView?.createDrapeEngine()
    }
    updateVisibleViewport()
  }

  func addMapView(_ mapView: EAGLView) {
    removeMapView()

    self.mapView = mapView
    placeholderView.isHidden = true
    mapView.translatesAutoresizingMaskIntoConstraints = false
    mapView.frame = view.bounds
    view.insertSubview(mapView, at: 0)
    NSLayoutConstraint.activate([
      mapView.topAnchor.constraint(equalTo: view.topAnchor),
      mapView.bottomAnchor.constraint(equalTo: view.bottomAnchor),
      mapView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      mapView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
    ])
  }

  func removeMapView() {
    mapView?.removeFromSuperview()
    mapView = nil
    speedInfoView.isHidden = true
  }

  func setPhoneModePlaceholderVisible(_ isVisible: Bool) {
    isPhoneModePlaceholderVisible = isVisible
    guard isViewLoaded else { return }
    placeholderView.isHidden = !isVisible
  }

  func updateVisibleViewport() {
    guard CarPlayService.shared.isCarplayActivated, let mapView else { return }
    let frame = view.bounds.inset(by: view.safeAreaInsets)
    FrameworkHelper.setVisibleViewport(frame, scaleFactor: mapView.contentScaleFactor)
  }

  func hideSpeedControl() {
    speedInfoView.isHidden = true
  }

  func showSpeedControl() {
    speedInfoView.isHidden = false
  }

  func updateCurrentSpeed(_ speedMps: Double, speedLimitMps: Double?) {
    speedInfoView.updateCurrentSpeed(speedMps, speedLimitMps: speedLimitMps)
  }

  func updateCameraInfo(isCameraOnRoute: Bool, speedLimitMps: Double?) {
    speedInfoView.updateCameraInfo(isCameraOnRoute: isCameraOnRoute, speedLimitMps: speedLimitMps)
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    ThemeManager.invalidate()
  }

  override func applyTheme() {
    super.applyTheme()
    updatePlaceholderAppearance()
    speedInfoView.updateAppearance()
  }

  private func setupPlaceholderView() {
    placeholderView.isHidden = !isPhoneModePlaceholderVisible
    placeholderView.isAccessibilityElement = true
    placeholderView.accessibilityLabel = "\(L("car_used_on_the_phone_screen")). \(L("car_continue_in_the_car"))"
    placeholderView.accessibilityTraits = .button
    placeholderView.addTarget(self, action: #selector(openMainCarPlay), for: .touchUpInside)
    placeholderView.translatesAutoresizingMaskIntoConstraints = false
    view.insertSubview(placeholderView, at: 0)

    placeholderDescriptionLabel.setFontStyle(.bold24)
    placeholderDescriptionLabel.text = L("car_used_on_the_phone_screen")
    placeholderDescriptionLabel.textAlignment = .center
    placeholderDescriptionLabel.numberOfLines = 0

    placeholderActionLabel.text = L("car_continue_in_the_car")
    placeholderActionLabel.setFontStyle(.semibold16)
    placeholderActionLabel.textAlignment = .center
    placeholderActionLabel.numberOfLines = 0

    placeholderActionView.isUserInteractionEnabled = false
    placeholderActionView.layer.cornerRadius = 12
    placeholderActionView.layer.masksToBounds = true
    placeholderActionView.addSubview(placeholderActionLabel)

    let contentStack = UIStackView(arrangedSubviews: [placeholderDescriptionLabel, placeholderActionView])
    contentStack.axis = .vertical
    contentStack.alignment = .fill
    contentStack.distribution = .fill
    contentStack.spacing = 24
    contentStack.translatesAutoresizingMaskIntoConstraints = false
    placeholderView.addSubview(contentStack)

    placeholderDescriptionLabel.translatesAutoresizingMaskIntoConstraints = false
    placeholderActionView.translatesAutoresizingMaskIntoConstraints = false
    placeholderActionLabel.translatesAutoresizingMaskIntoConstraints = false

    NSLayoutConstraint.activate([
      placeholderView.topAnchor.constraint(equalTo: view.topAnchor),
      placeholderView.bottomAnchor.constraint(equalTo: view.bottomAnchor),
      placeholderView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      placeholderView.trailingAnchor.constraint(equalTo: view.trailingAnchor),

      contentStack.centerYAnchor.constraint(equalTo: placeholderView.safeAreaLayoutGuide.centerYAnchor),
      contentStack.leadingAnchor.constraint(equalTo: placeholderView.safeAreaLayoutGuide.leadingAnchor, constant: 24),
      contentStack.trailingAnchor.constraint(equalTo: placeholderView.safeAreaLayoutGuide.trailingAnchor, constant: -24),

      placeholderActionView.heightAnchor.constraint(greaterThanOrEqualToConstant: 48),
      placeholderActionLabel.topAnchor.constraint(equalTo: placeholderActionView.topAnchor, constant: 12),
      placeholderActionLabel.bottomAnchor.constraint(equalTo: placeholderActionView.bottomAnchor, constant: -12),
      placeholderActionLabel.leadingAnchor.constraint(equalTo: placeholderActionView.leadingAnchor, constant: 24),
      placeholderActionLabel.trailingAnchor.constraint(equalTo: placeholderActionView.trailingAnchor, constant: -24),
    ])

    updatePlaceholderAppearance()
  }

  @objc private func openMainCarPlay() {
    CarPlayService.shared.openMainCarPlay()
  }

  private func updatePlaceholderAppearance() {
    placeholderView.backgroundColor = .carplayPlaceholderBackground
    placeholderDescriptionLabel.textColor = .blackSecondaryText
    placeholderActionView.backgroundColor = .linkBlue
    placeholderActionLabel.textColor = .whitePrimaryText
  }
}
