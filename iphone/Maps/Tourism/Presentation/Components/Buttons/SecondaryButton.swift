import SwiftUI

struct SecondaryButton: View {
    var label: String
    var loading: Bool = false
    var icon: (() -> AnyView)? = nil
    var onClick: () -> Void
    
    var body: some View {
        Button(action: onClick) {
            HStack {
                if loading {
                    // Loading indicator (you can customize this part)
                    ProgressView()
                        .progressViewStyle(CircularProgressViewStyle())
                } else {
                    if let icon = icon {
                        icon()
                            .frame(width: 30, height: 30)
                    }
                    Text(label)
                        .font(.headline)
                        .fontWeight(.semibold)
                        .foregroundColor(Color.primary)
                        .padding()
                }
            }
            .padding()
            .background(Color.clear)
            .overlay(
                RoundedRectangle(cornerRadius: 16)
                    .stroke(Color.primary, lineWidth: 1)
            )
        }
        .padding()
    }
}
