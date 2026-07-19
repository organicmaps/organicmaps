import UIKit

final class CarPlaySpeedInfoView: UIView {
  @IBOutlet private var currentSpeedView: UIView!
  @IBOutlet private var currentSpeedLabel: UILabel!
  @IBOutlet private var speedCamLimitContainer: UIView!
  @IBOutlet private var speedCamImageView: UIImageView!
  @IBOutlet private var speedCamLimitLabel: UILabel!

  private var currentSpeedMps: Double = 0
  private var speedLimitMps: Double?
  private var speedCamLimitMps: Double?
  private var isCameraOnRoute = false
  private var isSpeedCamBlinking = false

  override var isHidden: Bool {
    didSet {
      updateBlinkingState()
    }
  }

  override init(frame: CGRect) {
    super.init(frame: frame)
    setupProgrammaticLayout()
    configureView()
  }

  required init?(coder: NSCoder) {
    super.init(coder: coder)
  }

  override func awakeFromNib() {
    super.awakeFromNib()
    configureView()
  }

  override func didMoveToWindow() {
    super.didMoveToWindow()
    updateBlinkingState()
  }

  override var intrinsicContentSize: CGSize {
    CGSize(width: 30, height: 60)
  }

  func updateCurrentSpeed(_ speedMps: Double, speedLimitMps: Double?) {
    currentSpeedMps = speedMps
    self.speedLimitMps = speedLimitMps
    updateAppearance()
  }

  func updateCameraInfo(isCameraOnRoute: Bool, speedLimitMps: Double?) {
    self.isCameraOnRoute = isCameraOnRoute
    speedCamLimitMps = speedLimitMps
    updateAppearance()
  }

  func updateAppearance() {
    currentSpeedLabel.text = Measure(asSpeed: currentSpeedMps).valueAsString

    if isCameraOnRoute {
      speedCamLimitContainer.layer.borderColor = UIColor.speedLimitRed.cgColor
      speedCamImageView.tintColor = .speedLimitRed

      // Prefer the camera-specific limit and fall back to the current road limit when it is unknown.
      if let cameraSpeedLimitMps = speedCamLimitMps ?? speedLimitMps {
        speedCamLimitLabel.text = Measure(asSpeed: cameraSpeedLimitMps).valueAsString
        speedCamLimitLabel.textColor = .speedLimitDarkGray
        currentSpeedLabel.textColor = .whitePrimary
        currentSpeedView.backgroundColor = cameraSpeedLimitMps >= currentSpeedMps ? .speedLimitGreen : .speedLimitRed
      } else {
        speedCamLimitLabel.alpha = 0
        speedCamImageView.alpha = 1
        currentSpeedLabel.textColor = .speedLimitDarkGray
        currentSpeedView.backgroundColor = .speedLimitWhite
      }
    } else {
      currentSpeedLabel.textColor = .speedLimitDarkGray
      if let speedLimitMps {
        speedCamImageView.alpha = 0
        let speedLimitMeasure = Measure(asSpeed: speedLimitMps)
        speedCamLimitLabel.textColor = .speedLimitDarkGray
        // A zero road limit is the routing engine's representation of an unlimited road.
        speedCamLimitLabel.text = speedLimitMeasure.value == 0 ? "🚀" : speedLimitMeasure.valueAsString
        speedCamLimitLabel.alpha = 1
        speedCamLimitContainer.layer.borderColor = UIColor.speedLimitRed.cgColor
        if currentSpeedMps > speedLimitMps {
          currentSpeedLabel.textColor = .speedLimitRed
        }
      } else {
        speedCamImageView.tintColor = .speedLimitLightGray
        speedCamImageView.alpha = 1
        speedCamLimitLabel.alpha = 0
        speedCamLimitContainer.layer.borderColor = UIColor.speedLimitLightGray.cgColor
      }
      currentSpeedView.backgroundColor = .speedLimitWhite
    }
    updateBlinkingState()
  }

  private func configureView() {
    speedCamLimitContainer.layer.borderWidth = 2
    updateAppearance()
  }

  private func setupProgrammaticLayout() {
    currentSpeedView = UIView()
    currentSpeedLabel = UILabel()
    speedCamLimitContainer = UIView()
    speedCamImageView = UIImageView(image: UIImage(named: "ic_carplay_camera"))
    speedCamLimitLabel = UILabel()

    currentSpeedView.translatesAutoresizingMaskIntoConstraints = false
    currentSpeedView.clipsToBounds = true
    currentSpeedView.layer.cornerRadius = 15
    addSubview(currentSpeedView)

    setupLabel(currentSpeedLabel)
    currentSpeedView.addSubview(currentSpeedLabel)

    speedCamLimitContainer.translatesAutoresizingMaskIntoConstraints = false
    speedCamLimitContainer.clipsToBounds = true
    speedCamLimitContainer.backgroundColor = .white
    speedCamLimitContainer.layer.cornerRadius = 15
    addSubview(speedCamLimitContainer)

    speedCamImageView.translatesAutoresizingMaskIntoConstraints = false
    speedCamImageView.contentMode = .center
    speedCamLimitContainer.addSubview(speedCamImageView)

    setupLabel(speedCamLimitLabel)
    speedCamLimitContainer.addSubview(speedCamLimitLabel)

    NSLayoutConstraint.activate([
      currentSpeedView.leadingAnchor.constraint(equalTo: leadingAnchor),
      currentSpeedView.trailingAnchor.constraint(equalTo: trailingAnchor),
      currentSpeedView.bottomAnchor.constraint(equalTo: bottomAnchor),
      currentSpeedView.heightAnchor.constraint(equalToConstant: 55),
      currentSpeedLabel.leadingAnchor.constraint(equalTo: currentSpeedView.leadingAnchor),
      currentSpeedLabel.trailingAnchor.constraint(equalTo: currentSpeedView.trailingAnchor),
      currentSpeedLabel.bottomAnchor.constraint(equalTo: currentSpeedView.bottomAnchor, constant: -4),
      currentSpeedLabel.heightAnchor.constraint(equalToConstant: 30),
      speedCamLimitContainer.leadingAnchor.constraint(equalTo: leadingAnchor),
      speedCamLimitContainer.trailingAnchor.constraint(equalTo: trailingAnchor),
      speedCamLimitContainer.topAnchor.constraint(equalTo: topAnchor),
      speedCamLimitContainer.heightAnchor.constraint(equalToConstant: 30),
      speedCamImageView.leadingAnchor.constraint(equalTo: speedCamLimitContainer.leadingAnchor),
      speedCamImageView.trailingAnchor.constraint(equalTo: speedCamLimitContainer.trailingAnchor),
      speedCamImageView.topAnchor.constraint(equalTo: speedCamLimitContainer.topAnchor),
      speedCamImageView.bottomAnchor.constraint(equalTo: speedCamLimitContainer.bottomAnchor),
      speedCamLimitLabel.leadingAnchor.constraint(equalTo: speedCamLimitContainer.leadingAnchor),
      speedCamLimitLabel.trailingAnchor.constraint(equalTo: speedCamLimitContainer.trailingAnchor),
      speedCamLimitLabel.topAnchor.constraint(equalTo: speedCamLimitContainer.topAnchor),
      speedCamLimitLabel.bottomAnchor.constraint(equalTo: speedCamLimitContainer.bottomAnchor),
    ])
  }

  private func setupLabel(_ label: UILabel) {
    label.translatesAutoresizingMaskIntoConstraints = false
    label.backgroundColor = .clear
    label.setFontStyle(.bold14)
    label.textAlignment = .center
  }

  private func updateBlinkingState() {
    let hasCameraSpeedLimit = (speedCamLimitMps ?? speedLimitMps) != nil
    setSpeedCameraBlinking(isCameraOnRoute && hasCameraSpeedLimit && !isHidden && window != nil)
  }

  private func setSpeedCameraBlinking(_ isBlinking: Bool) {
    guard isBlinking != isSpeedCamBlinking else { return }

    if isBlinking {
      speedCamLimitLabel.alpha = 0
      speedCamImageView.alpha = 1
      UIView.animate(withDuration: 0.5,
                     delay: 0,
                     options: [.repeat, .autoreverse, .curveEaseOut],
                     animations: { [weak self] in
                       self?.speedCamImageView.alpha = 0
                       self?.speedCamLimitLabel.alpha = 1
                     })
    } else {
      speedCamLimitLabel.layer.removeAllAnimations()
      speedCamImageView.layer.removeAllAnimations()
    }

    isSpeedCamBlinking = isBlinking
  }
}
