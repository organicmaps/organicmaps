import SwiftUI
import Combine

struct AllReviewsScreen: View {
  @ObservedObject var reviewsVM: ReviewsViewModel
  @Environment(\.presentationMode) var presentationMode
  
  var body: some View {
    VStack(alignment: .leading) {
      BackButtonWithText {
        presentationMode.wrappedValue.dismiss()
      }
      ScrollView {
        LazyVStack(spacing: 16) {
          ForEach(reviewsVM.reviews, id: \.self) { review in
            ReviewView(review: review, onDeleteClick: nil)
          }
        }
      }
    }
    .padding(.horizontal, 16)
    .padding(.top, UIApplication.shared.statusBarFrame.height)
    .padding(.bottom, 48)
    .background(Color.background)
    .ignoresSafeArea()
  }
}

