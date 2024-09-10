import UIKit
import SwiftUI
import PhotosUI

class PersonalDataViewController: UIViewController {
  private var profileVM: ProfileViewModel
  
  init(profileVM: ProfileViewModel) {
    self.profileVM = profileVM
    super.init(
      nibName: nil,
      bundle: nil
    )
  }
  
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    integrateSwiftUIScreen(PersonalDataScreen(profileVM: profileVM))
  }
}

struct PersonalDataScreen: View {
  @ObservedObject var profileVM: ProfileViewModel
  @Environment(\.presentationMode) var presentationMode: Binding<PresentationMode>
  
  @State private var showImagePicker = false
  
  var body: some View {
    ScrollView {
      VStack(alignment: .leading) {
        AppTopBar(
          title: L("personal_data"),
          onBackClick: {
            presentationMode.wrappedValue.dismiss()
          }
        )
        VerticalSpace(height: 24)
        
        HStack(alignment: .center) {
          // pfp
          Group {
            if profileVM.isImagePickerUsed {
              Image(uiImage: profileVM.pfpToUpload).resizable()
            } else {
              LoadImageView(url: profileVM.pfpFromRemote?.absoluteString)
            }
          }
          .scaledToFill()
          .frame(width: 100, height: 100)
          .clipShape(Circle())
          
          Spacer().frame(width: 32)
          
          // photo picker
          Group {
            Image(systemName: "photo.badge.arrow.down")
              .foregroundColor(Color.onBackground)
            Spacer().frame(width: 8)
            
            Text(L("upload_photo"))
              .textStyle(TextStyle.h4)
          }
          .onTapGesture {
            showImagePicker = true
          }
        }
        
        VerticalSpace(height: 24)
        
        AppTextField(
          value: $profileVM.fullName,
          hint: L("full_name")
        )
        VerticalSpace(height: 16)
        
        AppTextField(
          value: $profileVM.email,
          hint: L("email")
        )
        VerticalSpace(height: 24)
        
        if let code = profileVM.countryCodeName {
          UICountryPickerView(
            code: code,
            onCountryChanged: { code in
              profileVM.setCountryCodeName(code)
            }
          )
            .frame(height: 56)
          VerticalSpace(height: 32)
        }
        
        PrimaryButton(
          label: L("save"),
          onClick: {
            profileVM.save()
          }
        )
      }
      .padding()
      .sheet(isPresented: $showImagePicker) {
        ImagePicker(sourceType: .photoLibrary, selectedImage: $profileVM.pfpToUpload)
      }
    }
    .overlay(
      Group {
        if profileVM.shouldShowMessageOnPersonalDataScreen {
          ToastView(message: profileVM.messageToShowOnPersonalDataScreen, isPresented: $profileVM.shouldShowMessageOnPersonalDataScreen)
            .padding(.bottom)
        }
      },
      alignment: .bottom
    )
    .background(Color.background)
  }
}

