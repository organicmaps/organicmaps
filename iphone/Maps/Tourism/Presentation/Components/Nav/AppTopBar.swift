import SwiftUI

struct AppTopBar: View {
  var title: String
  var onBackClick: (() -> Void)?
  var actions: [TopBarActionData] = []
  
  var body: some View {
    VStack(alignment: .leading) {
      HStack {
        if let onBackClick = onBackClick {
          AppBackButton(onBackClick: onBackClick)
            .padding(.bottom)
        }
        
        Spacer()
        
        HStack {
          ForEach(actions, id: \.id) { action in
            TopBarAction(
              iconName: action.iconName,
              color: action.color,
              onClick: action.onClick
            )
          }
        }
      }
      
      Text(title)
        .textStyle(TextStyle.h1)
        .foregroundColor(.primary)
        .padding(EdgeInsets(top: 0, leading: 0, bottom: 0, trailing: 16))
      
      VerticalSpace(height: 12)
    }
  }
}

struct TopBarActionData {
  let id = UUID()
  let iconName: String
  let color: SwiftUI.Color? = nil
  let onClick: () -> Void
}

struct TopBarAction: View {
  var iconName: String
  var color: SwiftUI.Color? = nil
  var onClick: () -> Void
  
  var body: some View {
    Button(action: onClick) {
      Image(systemName: iconName)
        .resizable()
        .frame(width: 24, height: 24)
        .foregroundColor(color ?? .primary)
    }
  }
}
