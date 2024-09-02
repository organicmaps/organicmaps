import SwiftUI

class HomeViewController: UIViewController {
  override func viewDidLoad() {
    super.viewDidLoad()
    
    let hostingController = UIHostingController(rootView: HomeScreen())

    addChild(hostingController)
    hostingController.view.frame = view.frame
    view.addSubview(hostingController.view)
    hostingController.didMove(toParent: self)
  }
}

struct HomeScreen: View {
  var body: some View {
    Text("kdfal;ksjf")
  }
}
