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
//    vc.delegate = self
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
  }

  override func viewDidLayoutSubviews() {
    super.viewDidLayoutSubviews()
    scrollView.contentInset = UIEdgeInsets(top: scrollView.height, left: 0, bottom: 0, right: 0)
  }

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)
//    let previewFrame = scrollView.convert(previewViewController.view.bounds, from: previewViewController.view)
//    UIView.animate(withDuration: kDefaultAnimationDuration) {
//      self.scrollView.contentOffset = CGPoint(x: 0, y: previewFrame.maxY - self.scrollView.height)
//    }
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

  }

  func previewDidPressSimilarHotels() {

  }
}

extension PlacePageViewController: PlacePageInfoViewControllerDelegate {
  func didPressCall() {
    guard let phoneUrl = placePageData.infoData.phoneUrl,
      UIApplication.shared.canOpenURL(phoneUrl) else { return }
    UIApplication.shared.open(phoneUrl, options: [:])
  }

  func didPressWebsite() {
    guard let website = placePageData.infoData.website,
      let url = URL(string: website) else { return }
    UIApplication.shared.open(url, options: [:])
  }

  func didPressEmail() {

  }

  func didPressLocalAd() {

  }
}

extension PlacePageViewController: WikiDescriptionViewControllerDelegate {
  func didPressMore() {

  }
}

extension PlacePageViewController: TaxiViewControllerDelegate {
  func didPressOrder() {

  }
}

extension PlacePageViewController: AddReviewViewControllerDelegate {
  func didRate(_ rating: UgcSummaryRatingType) {

  }
}

extension PlacePageViewController: PlacePageReviewsViewControllerDelegate {
  func didPressMoreReviews() {

  }
}

extension PlacePageViewController: PlacePageButtonsViewControllerDelegate {
  func didPressHotels() {

  }

  func didPressAddPlace() {

  }

  func didPressEditPlace() {

  }

  func didPressAddBusiness() {

  }
}

extension PlacePageViewController: HotelPhotosViewControllerDelegate {
  func didSelectItemAt(_ index: Int) {

  }
}

extension PlacePageViewController: HotelDescriptionViewControllerDelegate {
  func hotelDescriptionDidPressMore() {

  }
}

extension PlacePageViewController: HotelFacilitiesViewControllerDelegate {
  func facilitiesDidPressMore() {

  }
}

extension PlacePageViewController: HotelReviewsViewControllerDelegate {
  func hotelReviewsDidPressMore() {

  }
}

extension PlacePageViewController: CatalogSingleItemViewControllerDelegate {
  func catalogPromoItemDidPressView() {

  }

  func catalogPromoItemDidPressMore() {

  }
}

extension PlacePageViewController: CatalogGalleryViewControllerDelegate {
  func promoGalleryDidPressMore() {
    
  }

  func promoGalleryDidSelectItemAtIndex(_ index: Int) {

  }
}

extension PlacePageViewController: PlacePageBookmarkViewControllerDelegate {
  func bookmarkDidPressEdit() {

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
