import Combine

class ReviewsRepositoryImpl : ReviewsRepository {
  var reviewsPersistenceController: ReviewsPersistenceController
  var reviewsService: ReviewsService
  
  var reviewsResource: PassthroughSubject<[Review], ResourceError>
  var isThereReviewPlannedToPublishResource = PassthroughSubject<Bool, Never>()
  
  init(
    reviewsPersistenceController: ReviewsPersistenceController,
    reviewsService: ReviewsService,
    reviewsResource: PassthroughSubject<[Review], ResourceError>
  ) {
    self.reviewsPersistenceController = reviewsPersistenceController
    self.reviewsService = reviewsService
    
    self.reviewsResource = reviewsPersistenceController.reviewsForPlaceSubject
    reviewsPersistenceController.reviewsPlannedToPostSubject.sink { completion in } receiveValue: { reviews in
      self.isThereReviewPlannedToPublishResource.send(reviews.isEmpty)
    }
  }
  
  func observeReviewsForPlace(id: Int64) {
    reviewsPersistenceController.observeReviewsForPlace(placeId: id)
    
    Task {
      let reviewsDTO = try await reviewsService.getReviewsByPlaceId(id: id)
      let reviews = reviewsDTO.data.map { reviewDto in reviewDto.toReview() }
      
      reviewsPersistenceController.deleteAllPlaceReviews(placeId: id)
      reviewsPersistenceController.putReviews(reviews)
    }
  }
  
  
  func isThereReviewPlannedToPublish(for placeId: Int64) {
    reviewsPersistenceController.getReviewsPlannedToPost()
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
