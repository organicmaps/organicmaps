import SwiftUI
import SDWebImageSwiftUI

struct LoadImageView: View {
  let url: String?
  
  @State var isError = false
  
  var body: some View {
    if let urlString = url, urlString != "" {
      ZStack(alignment: .center) {
        WebImage(url: URL(string: urlString))
          .onSuccess(perform: { Image, data, cache in
            // delay is here to avoid any updates during ui update and stop seing messages about it
            DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
              self.isError = false
            }
          })
          .onFailure(perform: { isError in
            DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
              self.isError = true
            }
          })
          .resizable()
          .indicator(.activity)
          .scaledToFill()
          .frame(maxWidth: UIScreen.main.bounds.width, maxHeight: 150) // Constrain the width and height
          .clipped()
          .transition(.fade(duration: 0.2))
        if(isError) {
          Image(systemName: "exclamationmark.circle")
            .font(.system(size: 30))
            .background(SwiftUI.Color.clear)
            .foregroundColor(Color.hint)
            .clipShape(Circle())
        }
      }
    } else {
      Text(L("no_image"))
        .foregroundColor(Color.hint)
    }
  }
}

struct LoadImageView_Previews: PreviewProvider {
  static var previews: some View {
    LoadImageView(url: Constants.imageUrlExample)
  }
}
