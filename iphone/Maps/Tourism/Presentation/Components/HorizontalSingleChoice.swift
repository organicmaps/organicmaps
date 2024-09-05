import SwiftUI

struct SingleChoiceItem<T: Equatable & Hashable> : Identifiable {
  let id: T
  let label: String
}

struct HorizontalSingleChoice<T: Equatable & Hashable>: View {
  let items: [SingleChoiceItem<T>]
  @Binding var selected: SingleChoiceItem<T>?
  var onSelectedChanged: (SingleChoiceItem<T>) -> Void
  var selectedColor: SwiftUI.Color = Color.selected
  var unselectedColor: SwiftUI.Color = Color.background
  
  var body: some View {
    ScrollView(.horizontal, showsIndicators: false) {
      HStack() {
        HorizontalSpace(width: 16)
        ForEach(items, id: \.id) { item in
          SingleChoiceItemView(
            item: item,
            isSelected: item.id == selected?.id,
            onClick: {
              selected = item
              onSelectedChanged(item)
            },
            selectedColor: selectedColor,
            unselectedColor: unselectedColor
          )
          HorizontalSpace(width: 8)
        }
      }
    }
  }
}

struct SingleChoiceItemView<T: Equatable & Hashable>: View {
  let item: SingleChoiceItem<T>
  let isSelected: Bool
  let onClick: () -> Void
  let selectedColor: SwiftUI.Color
  let unselectedColor: SwiftUI.Color
  
  var body: some View {
    Text(item.label)
      .font(.medium(size: 16))
      .foregroundColor(isSelected ? Color.onSelected : Color.onBackground)
      .padding(12)
      .background(isSelected ? selectedColor : unselectedColor)
      .cornerRadius(16)
      .overlay(
        RoundedRectangle(cornerRadius: 16)
          .stroke(Color.border, lineWidth: 1)
      )
      .onTapGesture(perform: onClick)
  }
}

struct HorizontalSingleChoice_Previews: PreviewProvider {
  static var previews: some View {
    PreviewWrapper()
  }
  
  struct PreviewWrapper: View {
    @State private var selected: SingleChoiceItem<String>?
    let items = [
      SingleChoiceItem(id: "1", label: "Option 1"),
      SingleChoiceItem(id: "2", label: "Option 2"),
      SingleChoiceItem(id: "3", label: "Option 3")
    ]
    
    var body: some View {
      HorizontalSingleChoice(
        items: items,
        selected: $selected,
        onSelectedChanged: { _ in }
      )
    }
  }
}
