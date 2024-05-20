#!/usr/bin/env python3

import requests
import zipfile
import os
import shutil

def download_vulkan_validation_layers():
  # Step 1: Download zip archive
  url = "https://github.com/KhronosGroup/Vulkan-ValidationLayers/releases/download/vulkan-sdk-1.3.275.0/android-binaries-1.3.275.0.zip"
  download_path = "vulkan_sdk.zip"

  print("Downloading zip archive...")
  response = requests.get(url)
  with open(download_path, "wb") as f:
    f.write(response.content)
  print("Download complete.")

  # Step 2: Unzip archive
  unzip_dir = "vulkan_sdk"
  print("Unzipping archive...")
  with zipfile.ZipFile(download_path, "r") as zip_ref:
    zip_ref.extractall(unzip_dir)
  print("Unzip complete.")

  # Step 3: Rename unpacked folder to "jniLibs" and move it to "android/app/src/main"
  unpacked_folder = os.listdir(unzip_dir)[0]
  jniLibs_path = os.path.join(unzip_dir, "jniLibs")
  os.rename(os.path.join(unzip_dir, unpacked_folder), jniLibs_path)
  destination_path = "android/app/src/main"
  shutil.move(jniLibs_path, destination_path)
  print("Deploy complete.")

  # Clean up downloaded zip file and empty directories
  os.remove(download_path)
  os.rmdir(unzip_dir)
  print("Clean up complete.")


if __name__ == '__main__':
  download_vulkan_validation_layers()
