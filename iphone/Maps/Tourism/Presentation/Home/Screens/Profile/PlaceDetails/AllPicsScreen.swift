import SwiftUI

struct AllPicsScreen: View {
  let urls: [String]
  
  @Environment(\.presentationMode) var presentationMode
  
  let minWidth = UIScreen.main.bounds.width / 2 - 16
  let maxHeight = 150.0
  
  var body: some View {
    VStack(alignment: .leading) {
      BackButtonWithText {
        presentationMode.wrappedValue.dismiss()
      }
      ScrollView {
        LazyVGrid(
          columns: [
            GridItem(.flexible(minimum: minWidth, maximum: minWidth)),
            GridItem(.flexible(minimum: minWidth, maximum: minWidth))
          ],
          spacing: 16
        ) {
          ForEach(urls, id: \.self) { url in
            LoadImageView(url: url)
              .frame(maxWidth: minWidth, maxHeight: maxHeight)
              .clipShape(RoundedRectangle(cornerRadius: 8))
              .scaledToFill()
          }
        }
      }
    }
    .padding(.horizontal, 16)
    .padding(.top, statusBarHeight())
    .padding(.bottom, 48)
    .background(Color.background)
    .ignoresSafeArea()
  }
}

