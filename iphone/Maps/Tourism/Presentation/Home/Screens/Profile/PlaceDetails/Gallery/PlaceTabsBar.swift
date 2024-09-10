import SwiftUI

let tabBarShape = RoundedRectangle(cornerRadius: 50)

struct PlaceTabsBar: View {
  let tabTitles = [L("description_tourism"), L("gallery"), L("reviews")]

  @Binding var selectedTab: Int
  
  
  var body: some View {
    HStack {
      ForEach(0..<tabTitles.count, id: \.self) { index in
        TabButton(title: tabTitles[index], isSelected: selectedTab == index) {
          selectedTab = index
        }
        .frame(maxWidth: .infinity)
      }
    }
    .padding(8)
    .background(Color.surface)
    .clipShape(tabBarShape)
  }
  
  struct TabButton: View {
    let title: String
    let isSelected: Bool
    let action: () -> Void
    
    var body: some View {
      Button(action: action) {
        Text(title)
          .textStyle(TextStyle.b1)
          .padding(.vertical, 4)
          .padding(.horizontal, 6)
          .background(isSelected ? Color.selected : SwiftUI.Color.clear)
          .foregroundColor(isSelected ? Color.onSelected : Color.onSurface)
          .clipShape(tabBarShape)
      }
    }
  }
}
