import SwiftUI

struct AppBackButton: View {
  var onBackClick: () -> Void
  var tint: SwiftUI.Color = .primary
  
  var body: some View {
    Button(action: onBackClick) {
      Image(systemName: "chevron.left")
        .resizable()
        .frame(width: 24, height: 24)
        .foregroundColor(tint)
    }
  }
}
