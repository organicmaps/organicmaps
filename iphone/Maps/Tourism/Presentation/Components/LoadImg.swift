import SwiftUI
import SDWebImageSwiftUI

struct LoadImageView: View {
  let url: String?
  
  var body: some View {
    if let urlString = url {
      WebImage(url: URL(string: urlString))
        .resizable()
        .indicator(.activity)
        .scaledToFill()
        .transition(.fade(duration: 0.2))
    } else {
      Text(L("no_image"))
        .foregroundColor(Color.surface)
    }
  }
}

struct LoadImageView_Previews: PreviewProvider {
  static var previews: some View {
    LoadImageView(url: Constants.imageUrlExample)
  }
}
