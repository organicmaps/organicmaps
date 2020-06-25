# Firebase Crashlytics SDK

## Development

Follow the subsequent instructions to develop, debug, unit test, and
integration test FirebaseCrashlytics:

### Prereqs

- At least CocoaPods 1.6.0
- Install [cocoapods-generate](https://github.com/square/cocoapods-generate)
- For nanopb and GDT:
    - `brew install protobuf nanopb-generator`
    - `easy_install protobuf python`

### To Develop

- Run `Crashlytics/generate_project.sh`
- `open gen/FirebaseCrashlytics/FirebaseCrashlytics.xcworkspace`

You're now in an Xcode workspace generate for building, debugging and
testing the FirebaseCrashlytics CocoaPod.

### Running Unit Tests

Open the generated workspace, choose the FirebaseCrashlytics-Unit-unit scheme and press Command-u.

### Changing crash report uploads (using GDT)

#### Update report proto

If the crash report proto needs to be updated, follow these instructions:

- Update `ProtoSupport/Protos/crashlytics.proto` with the new changes
- Depending on the type of fields added/removed, also update `ProtoSupport/Protos/crashlytics.options`.
 `CALLBACK` type fields in crashlytics.nanopb.c needs to be changed to `POINTER`
 (through the options file). Known field types that require an entry in crashlytics.options are
 `strings`, `repeated` and `bytes`.
- Run `generate_project.sh` to update the nanopb .c/.h files.