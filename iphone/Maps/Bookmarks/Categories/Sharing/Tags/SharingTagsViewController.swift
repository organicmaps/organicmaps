
protocol SharingTagsViewControllerDelegate: AnyObject {
  func sharingTagsViewController(_ viewController: SharingTagsViewController, didSelect tags: [MWMTag])
  func sharingTagsViewControllerDidCancel(_ viewController: SharingTagsViewController)
}

final class SharingTagsViewController: MWMViewController {
  @IBOutlet private weak var loadingTagsView: UIView!
  @IBOutlet private weak var progressView: UIView!
  @IBOutlet private weak var collectionView: UICollectionView!
  @IBOutlet private weak var doneButton: UIBarButtonItem!
  @IBOutlet private weak var descriptionLabel: UILabel! {
    didSet {
      descriptionLabel.text = ""
    }
  }
  
  private lazy var progress: MWMCircularProgress = {
    return MWMCircularProgress(parentView: progressView)
  }()
  
  let dataSource = TagsDataSource()
  weak var delegate: SharingTagsViewControllerDelegate?
  var isSelectionAvailable = true
  
  let kTagCellIdentifier = "TagCellIdentifier"
  let kTagHeaderIdentifier = "TagHeaderIdentifier"
  
  override func viewDidLoad() {
    super.viewDidLoad()

    title = L("ugc_route_tags_screen_label")
    descriptionLabel.text = String(coreFormat: L("ugc_route_tags_desc"),
                                   arguments:[dataSource.maxNumberOfTagsToSelect])

    doneButton.isEnabled = false
    navigationItem.rightBarButtonItem = doneButton
    navigationItem.leftBarButtonItem = UIBarButtonItem(barButtonSystemItem: .cancel,
                                                       target: self,
                                                       action: #selector(onCancel))
    
    collectionView.allowsMultipleSelection = true
    collectionView.contentInset = UIEdgeInsets(top: 16, left: 16, bottom: 16, right: 16)

    loadTags()
  }
  
  func loadTags() {
    progress.state = .spinner
    self.loadingTagsView.isHidden = false
    
    dataSource.loadTags { success in
      self.progress.state = .normal
      self.loadingTagsView.isHidden = true
      
      if success {
        self.descriptionLabel.text = String(coreFormat: L("ugc_route_tags_desc"),
                                            arguments:[self.dataSource.maxNumberOfTagsToSelect])
        self.collectionView.reloadData()
      } else {
        MWMAlertViewController.activeAlert().presentTagsLoadingErrorAlert(okBlock: { [weak self] in
          self?.loadTags()
        }, cancel: { [weak self] in
          self?.onCancel()
        })
      }
    }
  }
  
  @IBAction func onDone(_ sender: Any) {
    guard let selectedItemsIndexes = collectionView.indexPathsForSelectedItems,
      !selectedItemsIndexes.isEmpty else {
        assert(false, "Done button should be disabled if there are no selected tags")
        return
    }
    
    delegate?.sharingTagsViewController(self, didSelect: dataSource.tags(for: selectedItemsIndexes))
  }
  
  @objc func onCancel() {
    delegate?.sharingTagsViewControllerDidCancel(self)
  }
 }

 extension SharingTagsViewController: UICollectionViewDataSource, UICollectionViewDelegate {

  func numberOfSections(in collectionView: UICollectionView) -> Int {
    return dataSource.tagGroupsCount
  }

  func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
    return dataSource.tagsCount(in: section)
  }

  func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
    let cell = collectionView.dequeueReusableCell(withReuseIdentifier: kTagCellIdentifier,for: indexPath) as! TagCollectionViewCell
    cell.update(with: dataSource.tag(in: indexPath.section, at: indexPath.item),
                enabled: cell.isSelected || isSelectionAvailable)
    
    //we need to do this because of bug - ios 12 doesnt apply layout to cells until scrolling
    if #available(iOS 12.0, *) {
      cell.layoutIfNeeded()
    }
  
    return cell
  }

  func collectionView(_ collectionView: UICollectionView,
                      viewForSupplementaryElementOfKind kind: String,
                      at indexPath: IndexPath) -> UICollectionReusableView {
    let supplementaryView = collectionView.dequeueReusableSupplementaryView(ofKind: kind,
                                                                            withReuseIdentifier: kTagHeaderIdentifier,
                                                                            for: indexPath) as! TagSectionHeaderView
    supplementaryView.update(with: dataSource.tagGroup(at: indexPath.section))
    return supplementaryView
  }
  
  func collectionView(_ collectionView: UICollectionView,
                      didSelectItemAt indexPath: IndexPath) {
    doneButton.isEnabled = true
    
    if collectionView.indexPathsForSelectedItems?.count == dataSource.maxNumberOfTagsToSelect {
      isSelectionAvailable = false
      reloadUnselectedCells()
    }
  }

  func collectionView(_ collectionView: UICollectionView,
                      didDeselectItemAt indexPath: IndexPath) {
    if let selectedItemsIndexes = collectionView.indexPathsForSelectedItems {
      doneButton.isEnabled = !selectedItemsIndexes.isEmpty
    } else {
      doneButton.isEnabled = false
    }
    
    isSelectionAvailable = true
    reloadUnselectedCells()
  }
  
  func reloadUnselectedCells() {
    if let selectedItemsIndexes = collectionView.indexPathsForSelectedItems {
      let visibleItemsIndexes = collectionView.indexPathsForVisibleItems
      let notSelectedItemsIndexes = visibleItemsIndexes.filter { !selectedItemsIndexes.contains($0) }
      UIView.performWithoutAnimation {
        collectionView.reloadItems(at: notSelectedItemsIndexes)
      }
    }
  }
  
  func collectionView(_ collectionView: UICollectionView,
                      shouldSelectItemAt indexPath: IndexPath) -> Bool {
    return isSelectionAvailable
  }
}
