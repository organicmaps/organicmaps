import SwiftUI

struct AppTextField: View {
  @Binding var value: String
  var hint: String
  var label: String? = nil
  var isError: Bool? = nil
  var textFieldHeight: CGFloat = 50
  var labelFontSize:CGFloat = 13
  var hintFontSize: CGFloat = 15
  var textSize: CGFloat = 17
  var errorColor: SwiftUI.Color = SwiftUI.Color.red
  var maxLines: Int = 1
  var leadingIcon: (() -> AnyView)? = nil
  var trailingIcon: (() -> AnyView)? = nil
  
  @State private var isFocused: Bool = false
  
  var body: some View {
    VStack(alignment: .leading) {
      ZStack(alignment: .leading) {
        // Main text field
        TextField("", text: $value, onEditingChanged: { isEditing in
          isFocused = isEditing
        }, onCommit: {
          isFocused = false
        })
        .font(.system(size: textSize))
        .frame(height: textFieldHeight)
        .padding(.leading, leadingIcon != nil ? 40 : 0)
        .padding(.trailing, trailingIcon != nil ? 40 : 0)
        .background(Color.background)
        .padding(.bottom, 5)
        .overlay(
          // Underline
          Rectangle()
            .frame(height: 1)
            .foregroundColor(colorForState())
            .padding(.top, textFieldHeight / 2)
        )
        
        // Label
        if let label = label {
          Text(label)
            .font(.system(size: labelFontSize))
            .foregroundColor(Color.onBackground)
            .padding(.bottom, 50)
        }
        
        // Hint text
        Text(hint)
          .font(.system(size: hintFontSize))
          .foregroundColor(value.isEmpty ? Color.hint : SwiftUI.Color.clear)
          .padding(.bottom, 5)
        
        if let leadingIcon = leadingIcon {
          leadingIcon()
            .frame(width: 30, height: 30)
            .padding(.leading, 10)
        }
        
        if let trailingIcon = trailingIcon {
          trailingIcon()
            .frame(width: 30, height: 30)
            .padding(.trailing, 10)
        }
      }
      .frame(height: textFieldHeight)
    }
  }
  
  private func colorForState() -> SwiftUI.Color {
    if isError != nil { return errorColor }
    return isFocused ? Color.primary : Color.onBackground
  }
}
