protocol PlacePageButtonsViewControllerDelegate: AnyObject {
  func didPressHotels()
  func didPressAddPlace()
  func didPressEditPlace()
  func didPressAddBusiness()
}

class PlacePageButtonsViewController: UIViewController {
//  @IBOutlet var bookingButton: UIButton!
  @IBOutlet var addPlaceButton: UIButton!
  @IBOutlet var editPlaceButton: UIButton!
//  @IBOutlet var addBusinessButton: UIButton!

  private var buttons: [UIButton?] {
//    [bookingButton, addPlaceButton, editPlaceButton, addBusinessButton]
    [addPlaceButton, editPlaceButton]
  }

  var buttonsData: PlacePageButtonsData!
  var buttonsEnabled = true {
    didSet {
      buttons.forEach {
        $0?.isEnabled = buttonsEnabled
      }
    }
  }

  weak var delegate: PlacePageButtonsViewControllerDelegate?
  
  override func viewDidLoad() {
    super.viewDidLoad()

//    bookingButton.isHidden = !buttonsData.showHotelDescription
    addPlaceButton.isHidden = !buttonsData.showAddPlace
    editPlaceButton.isHidden = !buttonsData.showEditPlace
//    addBusinessButton.isHidden = !buttonsData.showAddBusiness

    buttons.forEach {
      $0?.isEnabled = buttonsEnabled
    }
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
