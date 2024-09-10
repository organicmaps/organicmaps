import SwiftUI

struct RatingBarView: View {
  @Binding var rating: Double
  let size: CGFloat
  
  var body: some View {
    HStack(spacing: 4) {
      ForEach(0..<5) { index in
        Image(systemName: index < Int(rating) ? "star.fill" : "star")
          .resizable()
          .frame(width: size, height: size)
          .foregroundColor(Color.starYellow)
          .onTapGesture {
            rating = Double(index + 1)
          }
      }
    }
  }
}
