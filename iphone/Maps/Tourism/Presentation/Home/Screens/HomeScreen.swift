import SwiftUI

struct HomeScreen: View {
    var body: some View {
      VStack() {
          VStack(spacing: 10) {
              Text("Oh, Hi Mark!")
          }
          Spacer()
      }
    }
}

struct HomeScreen_Previews: PreviewProvider {
    static var previews: some View {
        HomeScreen()
    }
}
