import UIKit

class HomeViewController: UIViewController {
  
  private let label1: UILabel = {
    let label = UILabel()
    label.text = L("bookmark_list_description_hint")
    label.textAlignment = .center
    label.translatesAutoresizingMaskIntoConstraints = false
    label.textColor = Color.primary
    Font.applyStyle(to: label, style: Font.h1)
    return label
  }()
  
  private let label2: UILabel = {
    let label = UILabel()
    label.text = L("welcome_to_tjk")
    label.textAlignment = .center
    label.translatesAutoresizingMaskIntoConstraints = false
    label.textColor = Color.onBackground
    Font.applyStyle(to: label, style: Font.b1)
    return label
  }()
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    view.backgroundColor = Color.background
    
    view.addSubview(label1)
    view.addSubview(label2)
    
    NSLayoutConstraint.activate([
      label1.centerXAnchor.constraint(equalTo: view.centerXAnchor),
      label1.centerYAnchor.constraint(equalTo: view.centerYAnchor),
      label2.centerXAnchor.constraint(equalTo: view.centerXAnchor),
      label2.topAnchor.constraint(equalTo: label1.bottomAnchor, constant: 20)
    ])
  }
}
