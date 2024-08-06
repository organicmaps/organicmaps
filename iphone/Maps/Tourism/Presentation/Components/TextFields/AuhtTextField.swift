import UIKit

class AuthTextField: UITextField {
  private let underline: UIView = UIView()
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    setupTextField()
  }
  
  required init?(coder: NSCoder) {
    super.init(coder: coder)
    setupTextField()
  }
  
  private func setupTextField() {
    self.backgroundColor = .clear
    self.borderStyle = .none
    self.attributedPlaceholder = NSAttributedString(
      string: "Enter text here",
      attributes: [NSAttributedString.Key.foregroundColor: UIColor.white]
    )
    self.textColor = .white
    self.leftViewMode = .always
    self.font = UIFont.systemFont(ofSize: 16)
    self.keyboardType = .default
    self.returnKeyType = .done
    
    self.heightAnchor.constraint(equalToConstant: 50)
    
    addUnderline()
  }
  
  private func addUnderline() {
    underline.translatesAutoresizingMaskIntoConstraints = false
    underline.backgroundColor = .white
    self.addSubview(underline)
    
    NSLayoutConstraint.activate([
      underline.leadingAnchor.constraint(equalTo: self.leadingAnchor),
      underline.trailingAnchor.constraint(equalTo: self.trailingAnchor),
      underline.topAnchor.constraint(equalTo: self.bottomAnchor, constant: 8),
      underline.heightAnchor.constraint(equalToConstant: 1)
    ])
  }
}
