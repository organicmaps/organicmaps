import Combine

class ReviewsRepositoryImpl : ReviewsRepository {
  var reviewsResource = PassthroughSubject<[Review], ResourceError>()
  
  func getReviewsForPlace(id: Int64) {
    // TODO: cmon
  }
  
  var isThereReviewPlannedToPublishPassthroughSubject = PassthroughSubject<[Review], ResourceError>()
  
  func isThereReviewPlannedToPublish(id: Int64) {
    // TODO: cmon
  }
  
  func postReview(review: ReviewToPost) -> AnyPublisher<SimpleResponse, ResourceError> {
    // TODO: cmon
    return PassthroughSubject<SimpleResponse, ResourceError>().eraseToAnyPublisher()
  }
  
  func deleteReview(id: Int64) -> AnyPublisher<SimpleResponse, ResourceError> {
    // TODO: cmon
    return PassthroughSubject<SimpleResponse, ResourceError>().eraseToAnyPublisher()
  }
  
  func syncReviews() {
    // TODO: cmon
  }
}
