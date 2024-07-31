import UIKit
import SwiftUI

class AuthViewController: UIViewController {
  override func viewDidLoad() {
    super.viewDidLoad()
  }
  
  @IBSegueAction func embedSwiftUIView(_ coder: NSCoder) -> UIViewController? {
    return UIHostingController(coder: coder, rootView: WelcomeScreen())
  }
}
