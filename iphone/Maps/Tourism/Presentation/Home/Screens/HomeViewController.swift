import UIKit

class HomeViewController: UIViewController {
  
  private let label: UILabel = {
    let label = UILabel()
    label.text = "Hello, World!"
    label.textAlignment = .center
    label.translatesAutoresizingMaskIntoConstraints = false
    return label
  }()
  
  override func viewDidLoad() {
    super.viewDidLoad()
    
    view.backgroundColor = .white
    
    view.addSubview(label)
    
    NSLayoutConstraint.activate([
      label.centerXAnchor.constraint(equalTo: view.centerXAnchor),
      label.centerYAnchor.constraint(equalTo: view.centerYAnchor)
    ])
  }
}
