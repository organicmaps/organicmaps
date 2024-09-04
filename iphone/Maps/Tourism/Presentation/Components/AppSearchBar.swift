import SwiftUI

struct AppSearchBar: View {
  @Binding var query: String
  var onSearchClicked: ((String) -> Void)?
  var onClearClicked: () -> Void
  
  @State private var isActive = false
  
  var body: some View {
    HStack {
      Image(systemName: "magnifyingglass")
        .foregroundColor(Color.hint)
        .onTapGesture {
          onSearchClicked?(query)
        }
      
      TextField("Search", text: $query, onCommit: {
        onSearchClicked?(query)
      })
      .textFieldStyle(PlainTextFieldStyle())
      .font(.medium(size: 16))
      .foregroundColor(Color.onBackground)
      
      if !query.isEmpty {
        Button(action: onClearClicked) {
          Image(systemName: "xmark.circle.fill")
            .foregroundColor(Color.hint)
        }
      }
    }
    .padding()
    .background(Color.surface)
    .cornerRadius(16)
    .overlay(
      RoundedRectangle(cornerRadius: 16)
        .stroke(Color.border, lineWidth: 1)
    )
    .onTapGesture {
      isActive = true
    }
  }
}

struct AppSearchBar_Previews: PreviewProvider {
  static var previews: some View {
    AppSearchBar(
      query: .constant(""),
      onSearchClicked: { _ in },
      onClearClicked: {}
    )
    .previewLayout(.sizeThatFits)
    .padding()
  }
}
