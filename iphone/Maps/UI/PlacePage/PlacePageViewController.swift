class PlacePageScrollView: UIScrollView {
  override func point(inside point: CGPoint, with event: UIEvent?) -> Bool {
    return point.y > 0
  }
}

@objc class PlacePageViewController: UIViewController {
  @IBOutlet var scrollView: UIScrollView!
  @IBOutlet var stackView: UIStackView!
  @IBOutlet var actionBarContainerView: UIView!
  
  @objc var placePageData: PlacePageData!

  var rootViewController: MapViewController {
    MapViewController.shared()
  }

  lazy var previewViewController: PlacePagePreviewViewController = {
    let vc = storyboard!.instantiateViewController(ofType: PlacePagePreviewViewController.self)
    vc.placePagePreviewData = placePageData.previewData
    vc.delegate = self
    return vc
  } ()

  lazy var catalogSingleItemViewController: CatalogSingleItemViewController = {
    let vc = storyboard!.instantiateViewController(ofType: CatalogSingleItemViewController.self)
    vc.view.isHidden = true
    vc.delegate = self
    return vc
  } ()

  lazy var catalogGalleryViewController: CatalogGalleryViewController = {
    let vc = storyboard!.instantiateViewController(ofType: CatalogGalleryViewController.self)
    vc.view.isHidden = true
    vc.delegate = self
    return vc
  } ()

  lazy var wikiDescriptionViewController: WikiDescriptionViewController = {
    let vc = storyboard!.instantiateViewController(ofType: WikiDescriptionViewController.self)
    vc.view.isHidden = true
    vc.delegate = self
    return vc
  } ()

  lazy var bookmarkViewController: PlacePageBookmarkViewController = {
    let vc = storyboard!.instantiateViewController(ofType: PlacePageBookmarkViewController.self)
    vc.view.isHidden = true
    vc.delegate = self
    return vc
  } ()

  lazy var infoViewController: PlacePageInfoViewController = {
    let vc = storyboard!.instantiateViewController(ofType: PlacePageInfoViewController.self)
    vc.placePageInfoData = placePageData.infoData
    vc.delegate = self
    return vc
  } ()

  lazy var taxiViewController: TaxiViewController = {
    let vc = storyboard!.instantiateViewController(ofType: TaxiViewController.self)
    vc.taxiProvider = placePageData.taxiProvider
    vc.delegate = self
    return vc
  } ()

  lazy var ratingSummaryViewController: RatingSummaryViewController = {
    let vc = storyboard!.instantiateViewController(ofType: RatingSummaryViewController.self)
    vc.view.isHidden = true
    return vc
  } ()

  lazy var addReviewViewController: AddReviewViewController = {
    let vc = storyboard!.instantiateViewController(ofType: AddReviewViewController.self)
    vc.view.isHidden = true
    vc.delegate = self
    return vc
  } ()

  lazy var reviewsViewController: PlacePageReviewsViewController = {
    let vc = storyboard!.instantiateViewController(ofType: PlacePageReviewsViewController.self)
    vc.view.isHidden = true
    vc.delegate = self
    return vc
  } ()

  lazy var buttonsViewController: PlacePageButtonsViewController = {
    let vc = storyboard!.instantiateViewController(ofType: PlacePageButtonsViewController.self)
    vc.buttonsData = placePageData.buttonsData!
    vc.delegate = self
    return vc
  } ()

  lazy var hotelPhotosViewController: HotelPhotosViewController = {
    let vc = storyboard!.instantiateViewController(ofType: HotelPhotosViewController.self)
    vc.view.isHidden = true
    vc.delegate = self
    return vc
  } ()

  lazy var hotelDescriptionViewController: HotelDescriptionViewController = {
    let vc = storyboard!.instantiateViewController(ofType: HotelDescriptionViewController.self)
    vc.view.isHidden = true
    vc.delegate = self
    return vc
  } ()

  lazy var hotelFacilitiesViewController: HotelFacilitiesViewController = {
    let vc = storyboard!.instantiateViewController(ofType: HotelFacilitiesViewController.self)
    vc.view.isHidden = true
    vc.delegate = self
    return vc
  } ()

  lazy var hotelReviewsViewController: HotelReviewsViewController = {
    let vc = storyboard!.instantiateViewController(ofType: HotelReviewsViewController.self)
    vc.view.isHidden = true
    vc.delegate = self
    return vc
  } ()

  lazy var actionBarViewController: ActionBarViewController = {
    let vc = storyboard!.instantiateViewController(ofType: ActionBarViewController.self)
    vc.placePageData = placePageData
    vc.delegate = self
    return vc
  } ()

  override func viewDidLoad() {
    super.viewDidLoad()
    addToStack(previewViewController)

    if placePageData.isPromoCatalog {
      addToStack(catalogSingleItemViewController)
      addToStack(catalogGalleryViewController)
      placePageData.loadCatalogPromo { [weak self] in
        guard let self = self else { return }
        guard let catalogPromo = self.placePageData.catalogPromo else {
          if self.placePageData.wikiDescriptionHtml != nil {
            self.wikiDescriptionViewController.view.isHidden = false
          }
          return
        }
        if catalogPromo.promoItems.count == 1 {
          self.catalogSingleItemViewController.promoItem = catalogPromo.promoItems.first!
          self.catalogSingleItemViewController.view.isHidden = false
        } else {
          self.catalogGalleryViewController.promoData = catalogPromo
          self.catalogGalleryViewController.view.isHidden = false
          if self.placePageData.wikiDescriptionHtml != nil {
            self.wikiDescriptionViewController.view.isHidden = false
          }
        }
      }
    }

    addToStack(wikiDescriptionViewController)
    if let wikiDescriptionHtml = placePageData.wikiDescriptionHtml {
      wikiDescriptionViewController.descriptionHtml = wikiDescriptionHtml
      if placePageData.bookmarkData?.bookmarkDescription == nil && !placePageData.isPromoCatalog {
        wikiDescriptionViewController.view.isHidden = false
      }
    }

    addToStack(bookmarkViewController)
    if let bookmarkData = placePageData.bookmarkData {
      bookmarkViewController.bookmarkData = bookmarkData
      bookmarkViewController.view.isHidden = false
    }

    addToStack(hotelPhotosViewController)
    addToStack(hotelDescriptionViewController)
    addToStack(hotelFacilitiesViewController)
    addToStack(hotelReviewsViewController)

    addToStack(infoViewController)

    if placePageData.taxiProvider != .none {
      addToStack(taxiViewController)
    }

    if placePageData.previewData.showUgc {
      addToStack(ratingSummaryViewController)
      addToStack(addReviewViewController)
      addToStack(reviewsViewController)
      placePageData.loadUgc { [weak self] in
        if let self = self, let ugcData =  self.placePageData.ugcData {
          self.previewViewController.updateUgc(ugcData)

          if !ugcData.isTotalRatingEmpty {
            self.ratingSummaryViewController.ugcData = ugcData
            self.ratingSummaryViewController.view.isHidden = false
          }
          if ugcData.isUpdateEmpty {
            self.addReviewViewController.view.isHidden = false
          }
          if !ugcData.isEmpty {
            self.reviewsViewController.ugcData = ugcData
            self.reviewsViewController.view.isHidden = false
          }
          self.view.layoutIfNeeded()
          let previewFrame = self.scrollView.convert(self.previewViewController.view.bounds, from: self.previewViewController.view)
          UIView.animate(withDuration: kDefaultAnimationDuration) {
            self.scrollView.contentOffset = CGPoint(x: 0, y: previewFrame.maxY - self.scrollView.height)
          }
        }
      }
    }

    if placePageData.buttonsData != nil {
      addToStack(buttonsViewController)
    }

    placePageData.loadOnlineData { [weak self] in
      if let self = self, let bookingData = self.placePageData.hotelBooking {
        self.previewViewController.updateBooking(bookingData, rooms: self.placePageData.hotelRooms)
        self.stackView.layoutIfNeeded()
        UIView.animate(withDuration: kDefaultAnimationDuration) {
          if !bookingData.photos.isEmpty {
            self.hotelPhotosViewController.photos = bookingData.photos
            self.hotelPhotosViewController.view.isHidden = false
          }
          self.hotelDescriptionViewController.hotelDescription = bookingData.hotelDescription
          self.hotelDescriptionViewController.view.isHidden = false
          if bookingData.facilities.count > 0 {
            self.hotelFacilitiesViewController.facilities = bookingData.facilities
            self.hotelFacilitiesViewController.view.isHidden = false
          }
          if bookingData.reviews.count > 0 {
            self.hotelReviewsViewController.reviewCount = bookingData.scoreCount
            self.hotelReviewsViewController.totalScore = bookingData.score
            self.hotelReviewsViewController.reviews = bookingData.reviews
            self.hotelReviewsViewController.view.isHidden = false
          }
          self.stackView.layoutIfNeeded()
        }
      }
    }

    actionBarViewController.view.translatesAutoresizingMaskIntoConstraints = false
    actionBarContainerView.addSubview(actionBarViewController.view)
    NSLayoutConstraint.activate([
      actionBarViewController.view.leadingAnchor.constraint(equalTo: actionBarContainerView.leadingAnchor),
      actionBarViewController.view.topAnchor.constraint(equalTo: actionBarContainerView.topAnchor),
      actionBarViewController.view.trailingAnchor.constraint(equalTo: actionBarContainerView.trailingAnchor),
      actionBarViewController.view.bottomAnchor.constraint(equalTo: view.safeAreaLayoutGuide.bottomAnchor)
    ])

    MWMLocationManager.add(observer: self)
    if let lastLocation = MWMLocationManager.lastLocation() {
      onLocationUpdate(lastLocation)
    }
    if let lastHeading = MWMLocationManager.lastHeading() {
      onHeadingUpdate(lastHeading)
    }

    let bgView = UIView()
    bgView.backgroundColor = UIColor.white()
    stackView.insertSubview(bgView, at: 0)
    bgView.alignToSuperview()
    scrollView.decelerationRate = .fast
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    scrollView.contentInset = UIEdgeInsets(top: scrollView.height, left: 0, bottom: 0, right: 0)
  }

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)
    let previewFrame = self.scrollView.convert(self.previewViewController.view.bounds, from: self.previewViewController.view)
    UIView.animate(withDuration: kDefaultAnimationDuration) {
      self.scrollView.contentOffset = CGPoint(x: 0, y: previewFrame.maxY - self.scrollView.height)
    }
  }

  // MARK: private

  private func addToStack(_ viewController: UIViewController) {
    addChild(viewController)
    stackView.addArrangedSubview(viewController.view)
    viewController.didMove(toParent: self)
  }
}

extension PlacePageViewController: PlacePagePreviewViewControllerDelegate {
  func previewDidPressAddReview() {
    MWMPlacePageManagerHelper.showUGCAddReview(placePageData, rating: .none, from: .placePagePreview)
  }

  func previewDidPressSimilarHotels() {
    MWMPlacePageManagerHelper.searchSimilar()
  }
}

extension PlacePageViewController: PlacePageInfoViewControllerDelegate {
  func didPressCall() {
    MWMPlacePageManagerHelper.call(placePageData)
  }

  func didPressWebsite() {
    MWMPlacePageManagerHelper.openWebsite(placePageData)
  }

  func didPressEmail() {

  }

  func didPressLocalAd() {
    MWMPlacePageManagerHelper.openLocalAdsURL(placePageData)
  }
}

extension PlacePageViewController: WikiDescriptionViewControllerDelegate {
  func didPressMore() {
    MWMPlacePageManagerHelper.showPlaceDescription(placePageData.wikiDescriptionHtml)
  }
}

extension PlacePageViewController: TaxiViewControllerDelegate {
  func didPressOrder() {
    MWMPlacePageManagerHelper.orderTaxi(placePageData)
  }
}

extension PlacePageViewController: AddReviewViewControllerDelegate {
  func didRate(_ rating: UgcSummaryRatingType) {
    MWMPlacePageManagerHelper.showUGCAddReview(placePageData, rating: rating, from: .placePage)
  }
}

extension PlacePageViewController: PlacePageReviewsViewControllerDelegate {
  func didPressMoreReviews() {

  }
}

extension PlacePageViewController: PlacePageButtonsViewControllerDelegate {
  func didPressHotels() {
    MWMPlacePageManagerHelper.openDescriptionUrl(placePageData)
  }

  func didPressAddPlace() {
    MWMPlacePageManagerHelper.addPlace(placePageData.locationCoordinate)
  }

  func didPressEditPlace() {
    MWMPlacePageManagerHelper.editPlace()
  }

  func didPressAddBusiness() {
    MWMPlacePageManagerHelper.addBusiness()
  }
}

extension PlacePageViewController: HotelPhotosViewControllerDelegate {
  func didSelectItemAt(_ index: Int) {

  }
}

extension PlacePageViewController: HotelDescriptionViewControllerDelegate {
  func hotelDescriptionDidPressMore() {
    MWMPlacePageManagerHelper.openMoreUrl(placePageData)
  }
}

extension PlacePageViewController: HotelFacilitiesViewControllerDelegate {
  func facilitiesDidPressMore() {
    MWMPlacePageManagerHelper.showAllFacilities(placePageData)
  }
}

extension PlacePageViewController: HotelReviewsViewControllerDelegate {
  func hotelReviewsDidPressMore() {
    MWMPlacePageManagerHelper.openReviewUrl(placePageData)
  }
}

extension PlacePageViewController: CatalogSingleItemViewControllerDelegate {
  func catalogPromoItemDidPressView() {
    MWMPlacePageManagerHelper.openCatalogSingleItem(placePageData, at: 0)
  }

  func catalogPromoItemDidPressMore() {
    MWMPlacePageManagerHelper.openCatalogSingleItem(placePageData, at: 0)
  }
}

extension PlacePageViewController: CatalogGalleryViewControllerDelegate {
  func promoGalleryDidPressMore() {
    MWMPlacePageManagerHelper.openCatalogMoreItems(placePageData)
  }

  func promoGalleryDidSelectItemAtIndex(_ index: Int) {
    MWMPlacePageManagerHelper.openCatalogSingleItem(placePageData, at: index)
  }
}

extension PlacePageViewController: PlacePageBookmarkViewControllerDelegate {
  func bookmarkDidPressEdit() {
    MWMPlacePageManagerHelper.editBookmark()
  }
}

extension PlacePageViewController: ActionBarViewControllerDelegate {
  func actionBarDidPressButton(_ type: ActionBarButtonType) {
    switch type {
    case .booking:
      MWMPlacePageManagerHelper.book(placePageData)
    case .bookingSearch:
      MWMPlacePageManagerHelper.searchSimilar()
    case .bookmark:
      if placePageData.bookmarkData != nil {
        MWMPlacePageManagerHelper.removeBookmark(placePageData)
      } else {
        MWMPlacePageManagerHelper.addBookmark(placePageData)
      }
    case .call:
      MWMPlacePageManagerHelper.call(placePageData)
    case .download:
      fatalError()
    case .opentable:
      fatalError("Opentable is not supported and will be deleted")
    case .partner:
      MWMPlacePageManagerHelper.openPartner(placePageData)
    case .routeAddStop:
      MWMPlacePageManagerHelper.routeAddStop(placePageData)
    case .routeFrom:
      MWMPlacePageManagerHelper.route(from: placePageData)
    case .routeRemoveStop:
      MWMPlacePageManagerHelper.routeRemoveStop(placePageData)
    case .routeTo:
      MWMPlacePageManagerHelper.route(to: placePageData)
    case .share:
      MWMPlacePageManagerHelper.share(placePageData)
    case .avoidToll:
      MWMPlacePageManagerHelper.avoidToll()
    case .avoidDirty:
      MWMPlacePageManagerHelper.avoidDirty()
    case .avoidFerry:
      MWMPlacePageManagerHelper.avoidFerry()
    case .more:
      fatalError("More button should've been handled in ActionBarViewContoller")
    @unknown default:
      fatalError()
    }
  }
}

extension PlacePageViewController: MWMLocationObserver {
  func onHeadingUpdate(_ heading: CLHeading) {
    if heading.trueHeading < 0 {
      return
    }

    let rad = heading.trueHeading * Double.pi / 180
    previewViewController.updateHeading(CGFloat(rad))
  }

  func onLocationUpdate(_ location: CLLocation) {
    let ppLocation = CLLocation(latitude: placePageData.locationCoordinate.latitude,
                                longitude: placePageData.locationCoordinate.longitude)
    let distance = location.distance(from: ppLocation)
    let distanceFormatter = MKDistanceFormatter()
    distanceFormatter.unitStyle = .abbreviated
    let formattedDistance = distanceFormatter.string(fromDistance: distance)
    previewViewController.updateDistance(formattedDistance)
  }

  func onLocationError(_ locationError: MWMLocationStatus) {
    
  }
}

extension UgcData: MWMReviewsViewModelProtocol {
  public func numberOfReviews() -> Int {
    reviews.count
  }

  public func review(with index: Int) -> MWMReviewProtocol {
    UgcReviewAdapter(reviews[index])
  }

  public func isExpanded(_ review: MWMReviewProtocol) -> Bool {
    false
  }

  public func markExpanded(_ review: MWMReviewProtocol) {

  }
}

class UgcReviewAdapter: MWMReviewProtocol {
  var date: String {
    let formatter = DateFormatter()
    formatter.dateStyle = .long
    formatter.timeStyle = .none
    return formatter.string(from: ugcReview.date)
  }

  var text: String {
    ugcReview.text
  }

  private let ugcReview: UgcReview
  init(_ review: UgcReview) {
    ugcReview = review
  }
}

