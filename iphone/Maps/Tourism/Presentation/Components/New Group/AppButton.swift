import UIKit

class AppButton: UIButton {
  
  private var originalButtonText: String?
  private var activityIndicator: UIActivityIndicatorView!
  
  var isLoading: Bool = false {
    didSet {
      updateLoadingState()
    }
  }
  
  private let highlightScale: CGFloat = 0.95
  
  init(label: String, isPrimary: Bool, icon: UIImage? = nil, target: Any?, action: Selector) {
    super.init(frame: .zero)
    
    originalButtonText = label
    setTitle(label, for: .normal)
    
    isPrimary ? setPrimaryAppearance() : setSecondaryAppearance()
    heightAnchor.constraint(equalToConstant: 56).isActive = true
    
    translatesAutoresizingMaskIntoConstraints = false
    
    addTarget(target, action: action, for: .touchUpInside)
    addTarget(self, action: #selector(handleTouchDown), for: .touchDown)
    addTarget(self, action: #selector(handleTouchUp), for: .touchUpInside)
    addTarget(self, action: #selector(handleTouchUp), for: .touchCancel)
    
    setupActivityIndicator()
    
    if let icon = icon {
      setImage(icon, for: .normal)
      imageEdgeInsets = UIEdgeInsets(top: 0, left: -8, bottom: 0, right: 8)
    }
  }
  
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  // MARK: Styles
  private func setPrimaryAppearance() {
    setTitleColor(.white, for: .normal)
    self.backgroundColor = UIKitColor.primary
    if let lab = self.titleLabel  {
      UIKitFont.applyStyle(to: lab, style: UIKitFont.h4)
    }
    layer.cornerRadius = 16
  }
  
  private func setSecondaryAppearance() {
    setTitleColor(.systemBlue, for: .normal)
    backgroundColor = .clear
    layer.cornerRadius = 16
    layer.borderWidth = 1
    layer.borderColor = UIColor.systemBlue.cgColor
  }
  
  // MARK: click animation
  @objc private func handleTouchDown() {
    animate(scale: highlightScale)
  }
  
  @objc private func handleTouchUp() {
    animate(scale: 1.0)
  }
  
  private func animate(scale: CGFloat) {
    UIView.animate(withDuration: 0.2, animations: {
      self.transform = CGAffineTransform(scaleX: scale, y: scale)
    })
  }
  
  // MARK: loading
  private func setupActivityIndicator() {
    activityIndicator = UIActivityIndicatorView(style: .white)
    activityIndicator.color = .white
    activityIndicator.hidesWhenStopped = true
    activityIndicator.translatesAutoresizingMaskIntoConstraints = false
    addSubview(activityIndicator)
    
    // Center the activity indicator in the button
    NSLayoutConstraint.activate([
      activityIndicator.centerXAnchor.constraint(equalTo: centerXAnchor),
      activityIndicator.centerYAnchor.constraint(equalTo: centerYAnchor)
    ])
  }
  
  private func updateLoadingState() {
    if isLoading {
      setTitle("", for: .normal)
      activityIndicator.startAnimating()
      isUserInteractionEnabled = false
    } else {
      setTitle(originalButtonText, for: .normal)
      activityIndicator.stopAnimating()
      isUserInteractionEnabled = true
    }
  }
}
