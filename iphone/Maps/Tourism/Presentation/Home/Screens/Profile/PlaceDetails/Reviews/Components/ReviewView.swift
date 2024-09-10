import SwiftUI
import SDWebImageSwiftUI

struct ReviewView: View {
  let review: Review
  let onDeleteClick: (() -> Void)?
  
  @State private var expandedComment = false
  
  var body: some View {
    VStack(alignment: .leading, spacing: 16) {
      Divider()
      
      HStack {
        UserView(user: review.user)
        Spacer()
        if review.deletionPlanned {
          Text(L("deletionPlanned"))
            .textStyle(TextStyle.b2)
            .foregroundColor(Color.onBackground)
        } else if let date = review.date {
          Text(date)
            .textStyle(TextStyle.b2)
            .foregroundColor(Color.onBackground)
        }
      }
      
      ReadOnlyRatingBarView(rating: Double(review.rating), size: 24)
      
      if !review.picsUrls.isEmpty {
        HStack(spacing: 8) {
          ForEach(Array(review.picsUrls.prefix(3).enumerated()), id: \.offset) { index, url in
            if index == 2 && review.picsUrls.count > 3 {
              NavigationLink(destination: AllPicsScreen(urls: review.picsUrls)) {
                ShowMoreView(url: url, remaining: review.picsUrls.count - 3)
              }
            } else {
              ReviewPicView(url: url)
            }
          }
        }
      }
      
      if let comment = review.comment, !comment.isEmpty {
        CommentView(comment: comment, expanded: $expandedComment)
      }
      
      if let onDeleteClick = onDeleteClick {
        Button(action: onDeleteClick) {
          Text(L("delete_review"))
            .foregroundColor(Color.heartRed)
        }
      }
    }
  }
}

struct UserView: View {
  let user: User
  
  var body: some View {
    HStack {
      if let pfpUrl = user.pfpUrl {
        WebImage(url: URL(string: pfpUrl))
          .resizable()
          .aspectRatio(contentMode: .fill)
          .frame(width: 66, height: 66)
          .clipShape(Circle())
      }
      HStack() {
        Text(user.name)
          .textStyle(TextStyle.h3)
          .foregroundColor(Color.onBackground)
        UICountryFlagView(code: user.countryCodeName)
          .scaledToFit()
          .frame(height: 30)
      }
      Spacer()
    }
  }
}

struct ReadOnlyRatingBarView: View {
  let rating: Double
  let size: CGFloat
  
  var body: some View {
    HStack(spacing: 4) {
      ForEach(0..<5) { index in
        Image(systemName: index < Int(rating) ? "star.fill" : "star")
          .resizable()
          .frame(width: size, height: size)
          .foregroundColor(Color.starYellow)
      }
    }
  }
}

struct CommentView: View {
  let comment: String
  @Binding var expanded: Bool
  
  var body: some View {
    VStack(alignment: .leading) {
      Text(comment)
        .textStyle(TextStyle.b1)
        .lineLimit(expanded ? nil : 2)
        .onTapGesture {
          expanded.toggle()
        }
      
      VerticalSpace(height: 16)
      
      if !expanded {
        Button(L("more")) { expanded.toggle() }
          .foregroundColor(Color.primary)
      } else {
        Button(L("less")) { expanded.toggle() }
          .foregroundColor(Color.primary)
      }
    }
    .padding()
    .background(Color.surface)
    .cornerRadius(10)
  }
}

let reviewPicWidth = 73.0
let reviewPicHeight = 65.0

struct ReviewPicView: View {
  let url: String
  
  var body: some View {
    WebImage(url: URL(string: url))
      .resizable()
      .aspectRatio(contentMode: .fill)
      .frame(width: reviewPicWidth, height: reviewPicHeight)
      .clipShape(RoundedRectangle(cornerRadius: 8))
  }
}

struct ShowMoreView: View {
  let url: String
  let remaining: Int
  
  var body: some View {
    ZStack {
      ReviewPicView(url: url)
      SwiftUI.Color.black.opacity(0.5)
      Text("+\(remaining)")
        .textStyle(TextStyle.h3)
        .foregroundColor(SwiftUI.Color.white)
    }
    .frame(width: reviewPicWidth, height: reviewPicHeight)
    .clipShape(RoundedRectangle(cornerRadius: 8))
  }
}
