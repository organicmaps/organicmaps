import SwiftUI

struct HorizontalPlaces: View {
  let title: String
  let items: [PlaceShort]
  let onPlaceClick: (PlaceShort) -> Void
  let setFavoriteChanged: (PlaceShort, Bool) -> Void
  
  var body: some View {
    VStack(alignment: .leading) {
      Text(title)
        .font(.semiBold(size: 24))
        .foregroundColor(Color.onBackground)
        .padding(.horizontal)
      
      ScrollView(.horizontal, showsIndicators: false) {
        HStack {
          HorizontalSpace(width: 16)
          
          ForEach(items, id: \.id) { place in
            Place(
              place: place,
              onPlaceClick: { onPlaceClick(place) },
              isFavorite: place.isFavorite,
              onFavoriteChanged: { isFavorite in
                setFavoriteChanged(place, isFavorite)
              }
            )
            HorizontalSpace(width: 16)
          }
        }
      }
    }
  }
}

struct Place: View {
  let place: PlaceShort
  let onPlaceClick: () -> Void
  let isFavorite: Bool
  let onFavoriteChanged: (Bool) -> Void
  
  let width = 230.0
  let height = 250.0
  
  var body: some View {
    ZStack() {
      // cover
      LoadImageView(url: place.cover)
        .frame(width: width, height: height)
    }
    .overlay(
      // title and rating
      HStack() {
        VStack(alignment: .leading) {
          Text(place.name)
            .font(.semiBold(size: 15))
            .foregroundColor(.white)
            .lineLimit(2)
          VerticalSpace(height: 4)
          
          HStack(alignment: .center) {
            Text(String(format: "%.1f", place.rating ?? 0.0))
              .font(.semiBold(size: 15))
              .foregroundColor(.white)
            Image(systemName: "star.fill")
              .resizable()
              .foregroundColor(Color.starYellow)
              .frame(width: 10, height: 10)
          }
        }
        .padding(12)
        
        Spacer()
      }
        .frame(width: width)
        .background(SwiftUI.Color.black.opacity(0.5)),
      alignment: .bottom
    )
    .overlay(
      // favorite button
      Button(action: {
        onFavoriteChanged(!isFavorite)
      }) {
        Image(systemName: isFavorite ? "heart.fill" : "heart")
          .foregroundColor(.white)
          .padding(12)
          .background(SwiftUI.Color.white.opacity(0.2))
          .clipShape(Circle())
      }
        .padding(12),
      alignment: .topTrailing
    )
    .frame(width: width, height: height)
    .clipShape(RoundedRectangle(cornerRadius: 16))
    .contentShape(Rectangle())
    .onTapGesture(perform: onPlaceClick)
  }
}


struct HorizontalPlaces_Previews: PreviewProvider {
  static var previews: some View {
    HorizontalPlaces(
      title: "Popular Places",
      items: [
        PlaceShort(id: 1, name: "Place 1", cover: "url1", rating: 4.5, excerpt: "yep, just a placeyep, just a placeyep, just a placeyep, just a placeyep, just a place", isFavorite: false),
        PlaceShort(id: 2, name: "Place 2", cover: "url2", rating: 4.0, excerpt: "yep, just a place", isFavorite: true)
      ],
      onPlaceClick: { _ in },
      setFavoriteChanged: { _, _ in }
    )
  }
}
