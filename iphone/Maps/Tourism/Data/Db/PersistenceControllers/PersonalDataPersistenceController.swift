import CoreData
import Combine

class PersonalDataPersistenceController {
  static let shared = PersonalDataPersistenceController()
  private let coreDataController: SingleEntityCoreDataController<PersonalDataEntity>
  
  private init() {
    coreDataController = SingleEntityCoreDataController()
  }
  
  var personalDataSubject: PassthroughSubject<PersonalDataEntity?, ResourceError> {
    return coreDataController.entitySubject
  }
  
  func observePersonalData() {
    let fetchRequest: NSFetchRequest<PersonalDataEntity> = PersonalDataEntity.fetchRequest()
    let sortDescriptor = NSSortDescriptor(key: "id", ascending: true)
    coreDataController.observeEntity(fetchRequest: fetchRequest, sortDescriptor: sortDescriptor)
  }
  
  func updatePersonalData(personalData: PersonalData) -> AnyPublisher<Void, ResourceError> {
    let fetchRequest: NSFetchRequest<PersonalDataEntity> = PersonalDataEntity.fetchRequest()
    return coreDataController.updateEntity(updateBlock: { entityToUpdate in
      entityToUpdate.id = personalData.id
      entityToUpdate.fullName = personalData.fullName
      entityToUpdate.country = personalData.country
      entityToUpdate.pfpUrl = personalData.pfpUrl
      entityToUpdate.email = personalData.email
      entityToUpdate.language = personalData.language
      entityToUpdate.theme = personalData.theme
    }, fetchRequest: fetchRequest)
  }
}
