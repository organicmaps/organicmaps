import UIKit
import Combine

class ForgotPasswordViewController: UIViewController {
  
  
  private var cancellables = Set<AnyCancellable>()
  private var authRepository = AuthRepositoryImpl(authService: AuthServiceImpl())
  
  private let backButton: BackButton = {
    let backButton = BackButton()
    backButton.translatesAutoresizingMaskIntoConstraints = false
    return backButton
  }()
  
  private let backgroundImageView: UIImageView = {
    let imageView = UIImageView()
    imageView.image = UIImage(named: "splash_background")
    imageView.contentMode = .scaleAspectFill
    imageView.translatesAutoresizingMaskIntoConstraints = false
    return imageView
  }()
  
  private let containerView: UIView = {
    let view = UIView()
    view.backgroundColor = .clear
    view.translatesAutoresizingMaskIntoConstraints = false
    view.layer.cornerRadius = 16
    return view
  }()
  
  private let blurView: UIVisualEffectView = {
    let blurEffect = UIBlurEffect(style: .light)
    let blurView = UIVisualEffectView(effect: blurEffect)
    blurView.translatesAutoresizingMaskIntoConstraints = false
    blurView.layer.cornerRadius = 16
    blurView.clipsToBounds = true
    return blurView
  }()
  
  private let titleLabel: UILabel = {
    let label = UILabel()
    label.text = L("send_email_for_password_reset")
    UIKitFont.applyStyle(to: label, style: UIKitFont.h3)
    label.textColor = .white
    label.textAlignment = .center
    label.numberOfLines = 2
    label.translatesAutoresizingMaskIntoConstraints = false
    return label
  }()
  
  private let emailTextField: AuthTextField = {
    let textField = AuthTextField()
    textField.placeholder = L("tourism_email")
    textField.keyboardType = .emailAddress
    textField.autocapitalizationType = .none
    textField.translatesAutoresizingMaskIntoConstraints = false
    return textField
  }()
  
  private lazy var sendButton: AppButton = {
    let button = AppButton(
      label: L("send"),
      isPrimary: true,
      icon: nil,
      target: self,
      action: #selector(sendButtonTapped)
    )
    return button
  }()
  
  private lazy var cancelButton: AppButton = {
    let button = AppButton(
      label: L("cancel"),
      isPrimary: false,
      icon: nil,
      target: self,
      action: #selector(cancelButtonTapped)
    )
    return button
  }()
  
  override func viewDidLoad() {
    super.viewDidLoad()
    setupViews()
  }
  
  private func setupViews() {
    view.addSubview(backgroundImageView)
    view.addSubview(backButton)
    view.addSubview(containerView)
    
    containerView.addSubview(blurView)
    containerView.addSubview(titleLabel)
    containerView.addSubview(emailTextField)
    containerView.addSubview(sendButton)
    containerView.addSubview(cancelButton)
    
    NSLayoutConstraint.activate([
      // Background Image
      backgroundImageView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      backgroundImageView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
      backgroundImageView.topAnchor.constraint(equalTo: view.topAnchor),
      backgroundImageView.bottomAnchor.constraint(equalTo: view.bottomAnchor),
      
      // Back Button
      backButton.leadingAnchor.constraint(equalTo: view.safeAreaLayoutGuide.leadingAnchor, constant: 16),
      backButton.topAnchor.constraint(equalTo: view.safeAreaLayoutGuide.topAnchor),
      
      // Container View
      containerView.leadingAnchor.constraint(equalTo: view.leadingAnchor, constant: 16),
      containerView.trailingAnchor.constraint(equalTo: view.trailingAnchor, constant: -16),
      containerView.centerYAnchor.constraint(equalTo: view.centerYAnchor, constant: -120),
      
      // Blur View
      blurView.leadingAnchor.constraint(equalTo: containerView.leadingAnchor),
      blurView.trailingAnchor.constraint(equalTo: containerView.trailingAnchor),
      blurView.topAnchor.constraint(equalTo: containerView.topAnchor),
      blurView.bottomAnchor.constraint(equalTo: containerView.bottomAnchor),
      
      // Title Label
      titleLabel.topAnchor.constraint(equalTo: containerView.topAnchor, constant: 32),
      titleLabel.leadingAnchor.constraint(equalTo: containerView.leadingAnchor, constant: 32),
      titleLabel.trailingAnchor.constraint(equalTo: containerView.trailingAnchor, constant: -32),
      
      // Email Text Field
      emailTextField.topAnchor.constraint(equalTo: titleLabel.bottomAnchor, constant: 32),
      emailTextField.leadingAnchor.constraint(equalTo: containerView.leadingAnchor, constant: 16),
      emailTextField.trailingAnchor.constraint(equalTo: containerView.trailingAnchor, constant: -16),
      
      // Send Button
      sendButton.topAnchor.constraint(equalTo: emailTextField.bottomAnchor, constant: 32),
      sendButton.leadingAnchor.constraint(equalTo: containerView.leadingAnchor, constant: 16),
      sendButton.widthAnchor.constraint(equalTo: containerView.widthAnchor, multiplier: 0.43),
      sendButton.heightAnchor.constraint(equalToConstant: 44),
      
      // Cancell Button
      cancelButton.topAnchor.constraint(equalTo: emailTextField.bottomAnchor, constant: 32),
      cancelButton.trailingAnchor.constraint(equalTo: containerView.trailingAnchor, constant: -16),
      cancelButton.widthAnchor.constraint(equalTo: containerView.widthAnchor, multiplier: 0.43),
      cancelButton.heightAnchor.constraint(equalToConstant: 44),
      
      containerView.bottomAnchor.constraint(equalTo: sendButton.bottomAnchor, constant: 32)
    ])
    
    backButton.addTarget(self, action: #selector(backButtonTapped), for: .touchUpInside)
  }
  
  // MARK: -  buttons listeners
  @objc private func backButtonTapped() {
    self.navigationController?.popViewController(animated: false)
  }
  
  @objc private func sendButtonTapped() {
    sendButton.isLoading = true
    authRepository.sendEmailForPasswordReset(email: emailTextField.text ?? "")
      .sink(receiveCompletion: { [weak self] completion in
        switch completion {
        case .finished:
          self?.showToast(message: L("we_sent_you_password_reset_email"))
          self?.sendButton.isLoading = false
        case .failure(let error):
          self?.showError(message: error.errorDescription)
        }
      }, receiveValue: { _ in }
      )
      .store(in: &cancellables)
  }
  
  @objc private func cancelButtonTapped() {
    self.navigationController?.popViewController(animated: false)
  }
  
  // MARK: - other functions
  private func showError(message: String) {
    sendButton.isLoading = false
    showAlert(title: L("error"), message: message)
  }
}
