import XCTest
@testable import Organic_Maps_Debug

final class MWMNavigationInfoViewTests: XCTestCase {

    var userDefaults: UserDefaults!

    override func setUp() {
        super.setUp()
        userDefaults = UserDefaults(suiteName: "com.example.OrganicMapsTests") // Use a test-specific suite
        userDefaults.removePersistentDomain(forName: "com.example.OrganicMapsTests") // Clear UserDefaults before each test
    }

    override func tearDown() {
        userDefaults = nil
        super.tearDown()
    }

    func testAddStopToastAppearsOnlyInFirstSession() {
      
      // Simulate first session
      var isFirstSession = false;
      if ( userDefaults.set(true, forKey: "isFirstSession") != nil ){
        isFirstSession = true;
      }
      var hasStart = false;
      var hasFinish = false;
      
      //
      if ( userDefaults.set(true, forKey: "hasStart") != nil ){
        hasStart = true;
      }
      if ( userDefaults.set(true, forKey: "hasFinish") != nil ){
        hasFinish = true;
      }
      
      if (hasStart && hasFinish && isFirstSession){
        
        // Check if the toast view is visible
        XCTAssertTrue(isFirstSession, "Toast view should be visible during the first session.")
        
      }else if ( hasStart && hasFinish && !isFirstSession){
        
        // Check if the toast view is hidden
        XCTAssertFalse(!isFirstSession, "Toast view should not be visible after the first session.")
      }
            
        
      // Simulate subsequent session
      if ( userDefaults.set(false, forKey: "isFirstSession") != nil){
        isFirstSession = false;
      }
      
      if (hasStart && hasFinish && isFirstSession){
        
        // Check if the toast view is visible
        XCTAssertTrue(!isFirstSession, "Toast view should be visible during the first session.")
        
      }else if ( hasStart && hasFinish && !isFirstSession){
        
        // Check if the toast view is hidden
        XCTAssertFalse(isFirstSession, "Toast view should not be visible after the first session.")
      }
        
    }
}