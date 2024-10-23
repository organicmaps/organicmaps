import Combine

class ReviewsRepositoryImpl : ReviewsRepository {
  private var cancellables = Set<AnyCancellable>()
  
  var reviewsPersistenceController: ReviewsPersistenceController
  var reviewsService: ReviewsService
  
  var reviewsResource: PassthroughSubject<[Review], ResourceError>
  var isThereReviewPlannedToPublishResource = PassthroughSubject<Bool, Never>()
  
  init(
    reviewsPersistenceController: ReviewsPersistenceController,
    reviewsService: ReviewsService
  ) {
    self.reviewsPersistenceController = reviewsPersistenceController
    self.reviewsService = reviewsService
    
    self.reviewsResource = reviewsPersistenceController.reviewsForPlaceSubject
    reviewsPersistenceController.reviewsPlannedToPostSubject.sink { _ in } receiveValue: {
      reviews in self.isThereReviewPlannedToPublishResource.send(reviews.isEmpty)
    }
    .store(in: &cancellables)
  }
  
  func observeReviewsForPlace(id: Int64) {
    reviewsPersistenceController.observeReviewsForPlace(placeId: id)
    
    Task {
      let reviewsDTO = try await reviewsService.getReviewsByPlaceId(id: id)
      let reviews = reviewsDTO.data.map { reviewDto in reviewDto.toReview() }
      
      self.reviewsPersistenceController.deleteAllPlaceReviews(placeId: id)
      self.reviewsPersistenceController.insertReviews(reviews)
    }
  }
  
  func checkIfThereIsReviewPlannedToPublish(for placeId: Int64) {
    reviewsPersistenceController.observeReviewsForPlace(placeId: placeId)
  }
  
  func postReview(review: ReviewToPost) -> AnyPublisher<SimpleResponse, ResourceError> {
    return Future<SimpleResponse, ResourceError> { promise in
      Task {
        if Reachability.isConnectedToNetwork() {
          do {
            let response = try await self.reviewsService.postReview(review: review.toReviewToPostDTO())
            self.updateReviewsForDb(id: review.placeId)
            promise(.success(SimpleResponse(message: response.message)))
          } catch let error as ResourceError {
            print(error)
            promise(.failure(error))
          }
        } else {
          // images files already were saved in viewmodel, so no need to save them here
          self.reviewsPersistenceController.insertReviewPlannedToPost(review)
          promise(.failure(ResourceError.errorToUser(message: L("review_will_be_published_when_online"))))
        }
      }
    }
    .eraseToAnyPublisher()
  }
  
  func deleteReview(id: Int64) -> AnyPublisher<SimpleResponse, ResourceError> {
    return Future<SimpleResponse, ResourceError> { promise in
      Task {
        do {
          let response = try await self.reviewsService.deleteReview(reviews: ReviewIdsDTO(feedbacks: [id]))
          self.reviewsPersistenceController.deleteReview(id: id)
          promise(.success(response))
        } catch let error as ResourceError {
          self.reviewsPersistenceController.markReviewForDeletion(id: id, deletionPlanned: true)
          promise(.failure(error))
        }
      }
    }
    .eraseToAnyPublisher()
  }
  
  func syncReviews() {
    Task {
      try await deleteReviewsPlannedForDeletion()
      try await publishReviewsPlannedToPost()
    }
  }
  
  private func deleteReviewsPlannedForDeletion() async throws {
    let reviews = reviewsPersistenceController.getReviewsPlannedForDeletion()
    
    if !reviews.isEmpty {
      let reviewsIds = reviews.map(\.id)
      _ = try await reviewsService.deleteReview(reviews: ReviewIdsDTO(feedbacks: reviewsIds))
      reviewsPersistenceController.deleteReviews(ids: reviewsIds)
    }
  }
  
  private func publishReviewsPlannedToPost() async throws {
    let reviewsPlannedToPostEntities = reviewsPersistenceController.getReviewsPlannedToPost()
    if !reviewsPlannedToPostEntities.isEmpty {
      let reviewsDTO = reviewsPlannedToPostEntities.map {$0.toReviewToPostDTO()}
      reviewsDTO.forEach { reviewDTO in
        Task {
          do {
            _ = try await reviewsService.postReview(review: reviewDTO)
            updateReviewsForDb(id: reviewDTO.placeId)
            reviewsPersistenceController.deleteReviewPlannedToPost(placeId: reviewDTO.placeId)
            try reviewDTO.images.forEach { URL in
              try FileManager.default.removeItem(at: URL)
            }
          } catch {
            print(error)
          }
        }
      }
    }
  }
  
  private func updateReviewsForDb(id: Int64) {
    Task {
      let reviewsDTO = try await reviewsService.getReviewsByPlaceId(id: id)
      if !reviewsDTO.data.isEmpty {
        reviewsPersistenceController.deleteAllPlaceReviews(placeId: id)
        let reviews = reviewsDTO.data.map{ $0.toReview() }
        reviewsPersistenceController.insertReviews(reviews)
      }
    }
  }
}
