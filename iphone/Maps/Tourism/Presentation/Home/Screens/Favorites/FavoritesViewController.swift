import SwiftUI

class FavoritesViewController: UIViewController {
  override func viewDidLoad() {
    super.viewDidLoad()
    
    integrateSwiftUIScreen(FavoritesScreen())
  }
}

struct FavoritesScreen: View {
  var body: some View {
    Text("favorites")
  }
}
