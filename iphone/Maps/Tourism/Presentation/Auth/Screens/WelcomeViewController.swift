import UIKit

class WelcomeViewController: UIViewController {
  
  // MARK: - UI Components
  
  private let backgroundImageView: UIImageView = {
    let imageView = UIImageView()
    imageView.image = UIImage(named: "splash_background")
    imageView.contentMode = .scaleAspectFill
    imageView.translatesAutoresizingMaskIntoConstraints = false
    return imageView
  }()
  
  private let languageLabel: UILabel = {
    let label = UILabel()
    label.text = L("current_language")
    label.textColor = .white
    UIKitFont.applyStyle(to: label, style: UIKitFont.h4)
    label.translatesAutoresizingMaskIntoConstraints = false
    return label
  }()
  
  private let globeIcon: UIImageView = {
    let imageView = UIImageView()
    imageView.image = UIImage(named: "globe")
    imageView.tintColor = .white
    imageView.translatesAutoresizingMaskIntoConstraints = false
    return imageView
  }()
  
  private let welcomeLabel: UILabel = {
    let label = UILabel()
    label.text = L("welcome_to_tjk")
    label.textColor = .white
    UIKitFont.applyStyle(to: label, style: UIKitFont.h1)
    applyWrapContent(label: label)
    label.translatesAutoresizingMaskIntoConstraints = false
    return label
  }()
  
  private let signInButton: AppButton = {
    let button = AppButton(
      label: L("sign_in"),
      isPrimary: true,
      target: self,
      action: #selector(signInClicked)
    )
    return button
  }()
  
  private let signUpButton: AppButton = {
    let button = AppButton(
      label: L("sign_up"),
      isPrimary: true,
      target: self,
      action: #selector(signUpClicked)
    )
    return button
  }()
  
  private let copyrightLabel: UILabel = {
    let label = UILabel()
    label.text = "©"
    label.textColor = .white
    UIKitFont.applyStyle(to: label, style: UIKitFont.h2)
    label.translatesAutoresizingMaskIntoConstraints = false
    return label
  }()
  
  private let organizationNameLabel: UILabel = {
    let label = UILabel()
    label.text = L("organization_name")
    label.textColor = .white
    UIKitFont.applyStyle(to: label, style: UIKitFont.h4)
    applyWrapContent(label: label)
    label.translatesAutoresizingMaskIntoConstraints = false
    return label
  }()
  
  // MARK: - Lifecycle
  
  override func viewDidLoad() {
    super.viewDidLoad()
    setupUI()
    isInternetAvailable()
  }
  
  // MARK: - Setup
  
  private func setupUI() {
    
    let gradientView = UIView(frame: CGRect(x: 0, y: view.height - 100, width: view.width, height: 100))
    let gradient = CAGradientLayer()
    gradient.frame = gradientView.bounds
    gradient.colors = [UIColor.clear.cgColor, UIColor.black.cgColor]
    gradientView.layer.insertSublayer(gradient, at: 0)
    
    view.addSubview(backgroundImageView)
    view.addSubview(languageLabel)
    view.addSubview(globeIcon)
    view.addSubview(welcomeLabel)
    view.addSubview(signInButton)
    view.addSubview(signUpButton)
    view.addSubview(gradientView)
    view.addSubview(copyrightLabel)
    view.addSubview(organizationNameLabel)
    
    let languageTapGesture = UITapGestureRecognizer(target: self, action: #selector(languageClicked))
    languageLabel.isUserInteractionEnabled = true
    languageLabel.addGestureRecognizer(languageTapGesture)
    
    NSLayoutConstraint.activate([
      // Background Image
      backgroundImageView.topAnchor.constraint(equalTo: view.topAnchor),
      backgroundImageView.bottomAnchor.constraint(equalTo: view.bottomAnchor),
      backgroundImageView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      backgroundImageView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
      
      // Language Selection Row
      languageLabel.topAnchor.constraint(equalTo: view.safeAreaLayoutGuide.topAnchor, constant: 16),
      languageLabel.centerXAnchor.constraint(equalTo: view.centerXAnchor),
      
      globeIcon.centerYAnchor.constraint(equalTo: languageLabel.centerYAnchor),
      globeIcon.leadingAnchor.constraint(equalTo: languageLabel.trailingAnchor, constant: 8),
      
      // Welcome Text
      welcomeLabel.bottomAnchor.constraint(equalTo: signInButton.topAnchor, constant: -24),
      welcomeLabel.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: 16),
      welcomeLabel.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -16),
      
      // Sign In and Sign Up Buttons
      signInButton.bottomAnchor.constraint(equalTo: copyrightLabel.topAnchor, constant: -24),
      signInButton.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: 16),
      signInButton.trailingAnchor.constraint(equalTo: view.centerXAnchor, constant: -8),
      signInButton.heightAnchor.constraint(equalToConstant: 44),
      
      signUpButton.bottomAnchor.constraint(equalTo: copyrightLabel.topAnchor, constant: -24),
      signUpButton.leadingAnchor.constraint(equalTo: view.centerXAnchor, constant: 8),
      signUpButton.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -16),
      signUpButton.heightAnchor.constraint(equalToConstant: 44),
      
      // Copyright and Organization Name
      copyrightLabel.bottomAnchor.constraint(equalTo: view.safeAreaLayoutGuide.bottomAnchor, constant: -20),
      copyrightLabel.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: 16),
      
      organizationNameLabel.centerYAnchor.constraint(equalTo: copyrightLabel.centerYAnchor),
      organizationNameLabel.leadingAnchor.constraint(equalTo: copyrightLabel.trailingAnchor, constant: 8),
      organizationNameLabel.trailingAnchor.constraint(lessThanOrEqualTo: view.trailingAnchor, constant: -16),
    ])
  }
  
  // MARK: - Actions
  @objc private func languageClicked() {
    navigateToLanguageSettings()
  }
  
  @objc private func signInClicked() {
    performSegue(withIdentifier: "Welcome2SignIn", sender: nil)
  }
  
  @objc private func signUpClicked() {
    performSegue(withIdentifier: "Welcome2SignUp", sender: nil)
  }
  
  private func isInternetAvailable() {
      let monitor = NWPathMonitor()
      let queue = DispatchQueue.global(qos: .background)

      monitor.pathUpdateHandler = { path in
          monitor.cancel() // Stop monitoring after checking
      }

      monitor.start(queue: queue)
  }
  
}
