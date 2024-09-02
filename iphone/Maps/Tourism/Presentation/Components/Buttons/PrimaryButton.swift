import SwiftUI

struct PrimaryButton: View {
  var label: String
  var onClick: () -> Void
  var isLoading: Bool = false
  var enabled: Bool = true
  
  var body: some View {
    Button(action: onClick) {
      HStack {
        if isLoading {
          ProgressView()
            .progressViewStyle(CircularProgressViewStyle())
        } else {
          Text(label)
            .font(.headline)
            .fontWeight(.bold)
            .foregroundColor(Color.onPrimary)
            .padding()
            .frame(maxWidth: .infinity)
        }
      }
      .frame(
        maxWidth: .infinity,
        minHeight: 50
      )
      .background(Color.primary)
      .cornerRadius(16)
    }
    .disabled(!enabled || isLoading)
  }
}
