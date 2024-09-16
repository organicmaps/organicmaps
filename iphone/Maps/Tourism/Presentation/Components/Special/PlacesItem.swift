import SwiftUI
import WebKit

struct PlacesItem: View {
  let place: PlaceShort
  let onPlaceClick: (PlaceShort) -> Void
  let onFavoriteChanged: (Bool) -> Void
  
  private let height: CGFloat = 130
  
  var body: some View {
    HStack {
      LoadImageView(url: place.cover)
        .frame(width: height, height: height)
        .clipShape(RoundedRectangle(cornerRadius: 20))
      
      VStack(alignment: .leading, spacing: 8) {
        HStack {
          Text(place.name)
            .font(.semiBold(size: 20))
            .foregroundColor(Color.onBackground)
            .lineLimit(1)
          
          Spacer()
          
          Button(action: {
            onFavoriteChanged(!place.isFavorite)
          }) {
            Image(systemName: place.isFavorite ? "heart.fill" : "heart")
              .foregroundColor(Color.heartRed)
          }
        }
        
        HStack {
          Text(String(format: "%.1f", place.rating ?? 0.0))
            .font(.regular(size: 14))
            .foregroundColor(Color.onBackground)
          Image(systemName: "star.fill")
            .foregroundColor(Color.starYellow)
            .font(.system(size: 12))
        }
        
        if let excerpt = place.excerpt {
          Text(excerpt)
            .font(.regular(size: 14))
            .foregroundColor(Color.onBackground)
            .lineLimit(3)
        }
      }
      .padding(8)
    }
    .frame(height: height)
    .background(Color.background)
    .cornerRadius(20)
    .overlay(
      RoundedRectangle(cornerRadius: 20)
        .stroke(Color.border, lineWidth: 1)
    )
    .onTapGesture {
      onPlaceClick(place)
    }
  }
}
