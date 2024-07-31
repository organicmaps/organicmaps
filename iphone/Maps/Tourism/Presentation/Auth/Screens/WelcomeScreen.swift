import SwiftUI

struct WelcomeScreen: View {
  var body: some View {
    NavigationView{
      VStack {
        Text("Welcome!")
      }
    }
  }
}

struct WelcomeScreen_Previews: PreviewProvider {
  static var previews: some View {
    WelcomeScreen()
  }
}
