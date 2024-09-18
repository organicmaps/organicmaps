import Network
import SystemConfiguration

class DataSyncer {
  private let reviewsRepository: ReviewsRepository
  private let placesRepository: PlacesRepository
  
  init(reviewsRepository: ReviewsRepository, placesRepository: PlacesRepository) {
    self.reviewsRepository = reviewsRepository
    self.placesRepository = placesRepository
  }
  
  private let monitor = NWPathMonitor()
  private let queue = DispatchQueue.global(qos: .background)
  
  var isConnected: Bool = false
  var isExpensive: Bool = false
  
  func startMonitoring() {
    monitor.pathUpdateHandler = { path in
      self.isConnected = path.status == .satisfied
      self.isExpensive = path.isExpensive
      
      if path.status == .satisfied {
        print("Connected to the internet.")
        self.reviewsRepository.syncReviews()
        self.placesRepository.syncFavorites()
      } else {
        print("No internet connection.")
      }
      
      if path.isExpensive {
        print("Connection is on an expensive network, like cellular.")
      }
    }
    
    monitor.start(queue: queue)
  }
  
  func stopMonitoring() {
    monitor.cancel()
  }
}


public class Reachability {
  
  class func isConnectedToNetwork() -> Bool {
    
    var zeroAddress = sockaddr_in(sin_len: 0, sin_family: 0, sin_port: 0, sin_addr: in_addr(s_addr: 0), sin_zero: (0, 0, 0, 0, 0, 0, 0, 0))
    zeroAddress.sin_len = UInt8(MemoryLayout.size(ofValue: zeroAddress))
    zeroAddress.sin_family = sa_family_t(AF_INET)
    
    let defaultRouteReachability = withUnsafePointer(to: &zeroAddress) {
      $0.withMemoryRebound(to: sockaddr.self, capacity: 1) {zeroSockAddress in
        SCNetworkReachabilityCreateWithAddress(nil, zeroSockAddress)
      }
    }
    
    var flags: SCNetworkReachabilityFlags = SCNetworkReachabilityFlags(rawValue: 0)
    if SCNetworkReachabilityGetFlags(defaultRouteReachability!, &flags) == false {
      return false
    }
    
    /* Only Working for WIFI
     let isReachable = flags == .reachable
     let needsConnection = flags == .connectionRequired
     
     return isReachable && !needsConnection
     */
    
    // Working for Cellular and WIFI
    let isReachable = (flags.rawValue & UInt32(kSCNetworkFlagsReachable)) != 0
    let needsConnection = (flags.rawValue & UInt32(kSCNetworkFlagsConnectionRequired)) != 0
    let ret = (isReachable && !needsConnection)
    
    return ret
    
  }
}
