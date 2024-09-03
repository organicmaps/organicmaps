import SwiftUI
import SDWebImageSwiftUI

struct LoadImageView: View {
  let url: String?
  
  @State var isError = false
  
  var body: some View {
    if let urlString = url {
      let errorImage = Image(systemName: "error_centered")
      WebImage(url: URL(string: urlString))
      .resizable()
      .onSuccess(perform: { image, data, cacheType in
        isError = false
      })
      .onFailure(perform: { error in
        isError = true
      })
      .indicator(.activity).scaledToFill()
      .transition(.fade(duration: 0.2))
    } else {
      Text(L("no_image"))
        .foregroundColor(Color.surface)
    }
  }
}

struct ContentView: View {
  let imageUrl = Constants.imageUrlExample
  
  var body: some View {
    LoadImageView(url: imageUrl)
  }
}

struct ContentView_Previews: PreviewProvider {
  static var previews: some View {
    ContentView()
  }
}
