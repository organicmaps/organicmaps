import SwiftUI
import SDWebImageSwiftUI

struct PlaceTopBar: View {
  let title: String
  let picUrl: String?
  let onBackClick: (() -> Void)?
  let isFavorite: Bool
  let onFavoriteChanged: (Bool) -> Void
  let onMapClick: () -> Void
  
  private let height: CGFloat = 160
  private let padding: CGFloat = 16
  
  var body: some View {
    ZStack {
      // Load image
      LoadImageView(url: picUrl)
        .frame(height: height)
        .clipShape(
          RoundedRectangle(cornerRadius: 20, style: .continuous)
        )
      
      // Black overlay with opacity
      SwiftUI.Color.black.opacity(0.3)
        .frame(height: height)
        .clipShape(
          RoundedRectangle(cornerRadius: 20, style: .continuous)
        )
      
      // Top actions: Back, Favorite, Map
      VStack {
        HStack {
          if let onBackClick = onBackClick {
            PlaceTopBarAction(
              iconName: "chevron.left",
              onClick: onBackClick
            )
          }
          
          Spacer()
          
          PlaceTopBarAction(
            iconName: isFavorite ? "heart.fill" : "heart",
            onClick: { onFavoriteChanged(!isFavorite) }
          )
          
          PlaceTopBarAction(
            iconName: "map",
            onClick: onMapClick
          )
        }
        .padding(.horizontal, padding)
        .padding(.top, 48)
        
        VerticalSpace(height: 32)
        
        // Title
        Text(title)
          .textStyle(TextStyle.h2)
          .foregroundColor(.white)
          .padding(.horizontal, padding)
          .padding(.bottom, padding)
          .lineLimit(1)
          .truncationMode(.tail)
          .frame(maxWidth: .infinity, alignment: .leading)
      }
    }
    .frame(maxWidth: .infinity, maxHeight: height)
  }
}

struct PlaceTopBarAction: View {
  let iconName: String
  let onClick: () -> Void
  
  var body: some View {
    Button(action: onClick) {
      Image(systemName: iconName)
        .resizable()
        .scaledToFit()
        .frame(width: 24, height: 24)
        .padding(8)
        .background(SwiftUI.Color.white.opacity(0.2))
        .clipShape(Circle())
        .foregroundColor(.white)
    }
  }
}

struct PlaceTopBar_Previews: PreviewProvider {
  static var previews: some View {
    PlaceTopBar(
      title: "Place Title",
      picUrl: "https://example.com/image.jpg",
      onBackClick: { print("Back clicked") },
      isFavorite: true,
      onFavoriteChanged: { isFavorite in print("Favorite changed: \(isFavorite)") },
      onMapClick: { print("Map clicked") }
    )
  }
}
