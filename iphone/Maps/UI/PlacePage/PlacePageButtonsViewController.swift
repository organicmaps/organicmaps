protocol PlacePageButtonsViewControllerDelegate: AnyObject {
  func didPressHotels()
  func didPressAddPlace()
  func didPressEditPlace()
  func didPressAddBusiness()
}

class PlacePageButtonsViewController: UIViewController {
  @IBOutlet var bookingButton: UIButton!
  @IBOutlet var addPlaceButton: UIButton!
  @IBOutlet var editPlaceButton: UIButton!
  @IBOutlet var addBusinessButton: UIButton!

  var buttonsData: PlacePageButtonsData!
  weak var delegate: PlacePageButtonsViewControllerDelegate?
  
  override func viewDidLoad() {
    super.viewDidLoad()

    bookingButton.isHidden = !buttonsData.showHotelDescription
    addPlaceButton.isHidden = !buttonsData.showAddPlace
    editPlaceButton.isHidden = !buttonsData.showEditPlace
    addBusinessButton.isHidden = !buttonsData.showAddBusiness

      // Do any additional setup after loading the view.
  }
    
  @IBAction func onBooking(_ sender: UIButton) {
    delegate?.didPressHotels()
  }

  @IBAction func onAddPlace(_ sender: UIButton) {
    delegate?.didPressAddPlace()
  }

  @IBAction func onEditPlace(_ sender: UIButton) {
    delegate?.didPressEditPlace()
  }

  @IBAction func onAddBusiness(_ sender: UIButton) {
    delegate?.didPressAddBusiness()
  }
}
