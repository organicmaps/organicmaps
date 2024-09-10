import SwiftUI

struct FlowStack<Data: RandomAccessCollection, Content: View>: View where Data.Element: Hashable {
  let data: Data
  let spacing: CGFloat
  let alignment: HorizontalAlignment
  @ViewBuilder let content: (Data.Element) -> Content
  
  @State private var availableWidth: CGFloat = 0
  
  var body: some View {
    ZStack(alignment: Alignment(horizontal: alignment, vertical: .center)) {
      SwiftUI.Color.clear
        .frame(height: 1)
        .readSize { size in
          availableWidth = size.width
        }
      
      _FlowStack(
        availableWidth: availableWidth,
        data: data,
        spacing: spacing,
        alignment: alignment,
        content: content
      )
    }
  }
}

private struct _FlowStack<Data: RandomAccessCollection, Content: View>: View where Data.Element: Hashable {
  let availableWidth: CGFloat
  let data: Data
  let spacing: CGFloat
  let alignment: HorizontalAlignment
  @ViewBuilder let content: (Data.Element) -> Content
  
  @State private var elementsSize: [Data.Element: CGSize] = [:]
  
  var body: some View {
    VStack(alignment: alignment, spacing: spacing) {
      ForEach(computeRows(), id: \.self) { rowElements in
        HStack(spacing: spacing) {
          ForEach(rowElements, id: \.self) { element in
            content(element)
              .fixedSize()
              .readSize { size in
                elementsSize[element] = size
              }
          }
        }
      }
    }
  }
  
  private func computeRows() -> [[Data.Element]] {
    var rows: [[Data.Element]] = [[]]
    var currentRow = 0
    var remainingWidth = availableWidth
    
    for element in data {
      let elementSize = elementsSize[element, default: CGSize(width: availableWidth, height: 1)]
      
      if remainingWidth - (elementSize.width + spacing) >= 0 {
        rows[currentRow].append(element)
      } else {
        currentRow += 1
        rows.append([element])
        remainingWidth = availableWidth
      }
      
      remainingWidth -= elementSize.width + spacing
    }
    
    return rows
  }
}

private extension View {
  func readSize(onChange: @escaping (CGSize) -> Void) -> some View {
    background(
      GeometryReader { geometryProxy in
        SwiftUI.Color.clear
          .preference(key: SizePreferenceKey.self, value: geometryProxy.size)
      }
    )
    .onPreferenceChange(SizePreferenceKey.self, perform: onChange)
  }
}

private struct SizePreferenceKey: PreferenceKey {
  static var defaultValue: CGSize = .zero
  static func reduce(value: inout CGSize, nextValue: () -> CGSize) {}
}
