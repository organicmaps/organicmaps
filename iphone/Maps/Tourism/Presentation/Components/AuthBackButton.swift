import UIKit

class BackButton: UIButton {
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    setupButton()
  }
  
  required init?(coder: NSCoder) {
    super.init(coder: coder)
    setupButton()
  }
  
  private func setupButton() {
    self.setTitle("←", for: .normal)
    self.setTitleColor(.white, for: .normal)
    self.titleLabel?.font = UIFont.systemFont(ofSize: 32)
    self.layer.borderColor = UIColor.clear.cgColor
    self.backgroundColor = .clear
  }
}
