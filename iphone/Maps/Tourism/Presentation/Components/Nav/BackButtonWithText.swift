import SwiftUI

struct BackButtonWithText: View {
  var onBackClick: () -> Void
  
  var body: some View {
    Button(action: onBackClick) {
      HStack {
        Image(systemName: "chevron.left")
          .resizable()
          .frame(width: 8, height: 16)
        Text(L("back"))
          .font(.body)
      }
    }
  }
}
