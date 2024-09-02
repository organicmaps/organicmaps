import SwiftUI

struct AppBackButton: View {
  var onBackClick: () -> Void
  var tint: SwiftUI.Color = .primary
  
  var body: some View {
    Button(action: onBackClick) {
      Image(systemName: "arrow.left")
        .scaleEffect(1.5)
        .foregroundColor(tint)
        .padding(4)
    }
  }
}
