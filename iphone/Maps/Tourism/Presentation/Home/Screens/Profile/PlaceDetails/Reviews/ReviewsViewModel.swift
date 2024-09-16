import Combine

class ReviewsViewModel: ObservableObject {
  private let cancellables = Set<AnyCancellable>()
  
  private let reviewsRepository: ReviewsRepository
  
  init(reviewsRepository: ReviewsRepository, id: Int64) {
    self.reviewsRepository = reviewsRepository
    
    observeReviews(id: id)
    
    // TODO: cmon get isThereReviewPlannedToPublish from DB
  }
  
  @Published var reviews: [Review] = [Constants.reviewExample]
  @Published var userReview: Review? = nil
  @Published var isThereReviewPlannedToPublish = false
  
  func observeReviews(id: Int64) {
    reviewsRepository.reviewsResource.sink { _ in } receiveValue: { reviews in
      self.reviews = reviews
    }

    
    reviewsRepository.observeReviewsForPlace(id: id)
  }
  
  func deleteReview() {
    // TODO: cmon
  }
}
