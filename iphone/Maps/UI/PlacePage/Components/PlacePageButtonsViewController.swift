protocol PlacePageButtonsViewControllerDelegate: AnyObject {
  func didPressAddPlace()
  func didPressEditPlace()
}

class PlacePageButtonsViewController: UIViewController {
  @IBOutlet var addPlaceButton: UIButton!
  @IBOutlet var editPlaceButton: UIButton!

  private var buttons: [UIButton?] {
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

    addPlaceButton.isHidden = !buttonsData.showAddPlace
    editPlaceButton.isHidden = !buttonsData.showEditPlace

    addPlaceButton.isEnabled = buttonsData.enableAddPlace
    editPlaceButton.isEnabled = buttonsData.enableEditPlace
  }

  @IBAction func onAddPlace(_ sender: UIButton) {
    delegate?.didPressAddPlace()
  }

  @IBAction func onEditPlace(_ sender: UIButton) {
    delegate?.didPressEditPlace()
  }
}
