import SwiftUI

struct ReviewsScreen: View {
  @ObservedObject var reviewsVM: ReviewsViewModel
  
  let placeId: Int64
  let rating: Double?
  
  init(
    reviewsVM: ReviewsViewModel,
    placeId: Int64,
    rating: Double?
  ) {
    self.reviewsVM = reviewsVM
    self.placeId = placeId
    self.rating = rating
  }
  
  @State private var showReviewSheet = false
  
  var body: some View {
    ScrollView {
      VStack {
        // overal rating
        HStack(alignment: .center) {
          Image(systemName: "star.fill")
            .resizable()
            .frame(width: 30, height: 30)
            .foregroundColor(Color.starYellow)
          
          if let rating = rating {
            Text("\(String(format: "%.1f", rating) )/5")
              .font(.system(size: 30))
          }
          
          Spacer()
          
          Button(L("compose_review")) {
            showReviewSheet = true
          }
          .foregroundColor(Color.primary)
        }
        VerticalSpace(height: 16)
        
        HStack {
          Spacer()
          
          NavigationLink(destination: AllReviewsScreen(reviewsVM: reviewsVM)) {
            Text(L("see_all_reviews"))
              .foregroundColor(Color.primary)
          }
        }
        
        // user review
        if let userReview = reviewsVM.userReview, !reviewsVM.isThereReviewPlannedToPublish {
          ReviewView(
            review: userReview,
            onDeleteClick: {
              reviewsVM.deleteReview()
            }
          )
        }
        // most recent recent review
        if let mostRecentReview = reviewsVM.latestReview {
          ReviewView(
            review: mostRecentReview,
            onDeleteClick: nil
          )
        }
      }
    }
    .padding(.horizontal, 16)
    .sheet(isPresented: $showReviewSheet) {
      PostReviewView(
        postReviewVM: PostReviewViewModel(
          reviewsRepository: ReviewsRepositoryImpl(
            reviewsPersistenceController: ReviewsPersistenceController.shared,
            reviewsService: ReviewsServiceImpl(userPreferences: UserPreferences.shared)
          )
        ),
        placeId: placeId,
        onPostReviewSuccess: {
          showReviewSheet = false
        }
      )
    }
  }
}
