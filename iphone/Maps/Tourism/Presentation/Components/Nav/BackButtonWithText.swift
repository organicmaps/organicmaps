import SwiftUI

struct BackButtonWithText: View {
    var onBackClick: () -> Void

    var body: some View {
        Button(action: onBackClick) {
            HStack {
                Image(systemName: "chevron.left")
                    .resizable()
                    .frame(width: 16, height: 16)
                Text("Back")
                    .font(.body)
            }
            .padding(.horizontal, 16)
            .padding(.vertical, 8)
        }
    }
}
