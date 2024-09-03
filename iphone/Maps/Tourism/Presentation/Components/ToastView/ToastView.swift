import SwiftUI

struct ToastView: View {
  let message: String
  @Binding var isPresented: Bool
  
  var body: some View {
    VStack {
      Text(message)
        .padding()
        .foregroundColor(Color.onSurface)
        .background(Color.surface)
        .cornerRadius(10)
        .shadow(radius: 5)
    }
    .onAppear {
      DispatchQueue.main.asyncAfter(deadline: .now() + 2) {
        withAnimation {
          isPresented = false
        }
      }
    }
  }
}
