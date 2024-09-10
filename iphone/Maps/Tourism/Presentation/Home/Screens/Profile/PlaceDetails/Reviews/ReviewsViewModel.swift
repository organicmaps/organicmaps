import Combine

class ReviewsViewModel: ObservableObject {
  @Published var reviews: [Review] = [Constants.reviewExample]
  @Published var userReview: Review? = nil
  @Published var isThereReviewPlannedToPublish = false
  
  func getReviews(id: Int64) {
    // TODO: cmon user review and all other reviews
  }
  
  func deleteReview() {
    // TODO: cmon
  }
  
  init() {
    // TODO: cmon get isThereReviewPlannedToPublish from DB
  }
}
