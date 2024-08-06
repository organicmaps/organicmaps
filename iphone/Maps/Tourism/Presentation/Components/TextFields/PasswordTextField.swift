import UIKit

class PasswordTextField: AuthTextField {
  
  private let toggleVisibilityButton: UIButton = {
    let button = UIButton(type: .custom)
    let eyeSlashImg = UIImage(named: "eye_slash")
    let eyeImage = UIImage(named: "eye")
    button.setImage(eyeSlashImg, for: .normal)
    button.setImage(eyeImage, for: .selected)
    button.tintColor = .white
    button.addTarget(self, action: #selector(togglePasswordVisibility), for: .touchUpInside)
    button.translatesAutoresizingMaskIntoConstraints = false
    return button
  }()
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    setupPasswordTextField()
  }
  
  required init?(coder: NSCoder) {
    super.init(coder: coder)
    setupPasswordTextField()
  }
  
  private func setupPasswordTextField() {
    self.isSecureTextEntry = true
    self.rightView = toggleVisibilityButton
    self.rightViewMode = .always
    
    // Ensure the button is laid out correctly
    self.addSubview(toggleVisibilityButton)
    NSLayoutConstraint.activate([
      toggleVisibilityButton.widthAnchor.constraint(equalToConstant: 24),
      toggleVisibilityButton.heightAnchor.constraint(equalToConstant: 24),
      toggleVisibilityButton.trailingAnchor.constraint(equalTo: self.trailingAnchor, constant: -8),
      toggleVisibilityButton.centerYAnchor.constraint(equalTo: self.centerYAnchor)
    ])
  }
  
  @objc private func togglePasswordVisibility() {
    self.isSecureTextEntry.toggle()
    toggleVisibilityButton.isSelected = !self.isSecureTextEntry
  }
  
  override func layoutSubviews() {
    super.layoutSubviews()
    // Adjust the frame of the right view if needed
    let buttonWidth: CGFloat = 24
    let padding: CGFloat = 8
    self.rightView?.frame = CGRect(
      x: self.bounds.width - buttonWidth - padding,
      y: (self.bounds.height - buttonWidth) / 2,
      width: buttonWidth,
      height: buttonWidth
    )
  }
}
