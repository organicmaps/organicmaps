ActionBarSherlock Release Process
=================================

 1. Make sure she builds!

        mvn clean verify

 2. Ensure the `CHANGELOG.md` file has up-to-date information and the current date.
 3. Pull in the latest translations in the i18n module.
 4. Change all of the sample `AndroidManifest.xml` files to the correct version and bump the
    version code arbitrarily.

        find actionbarsherlock-samples -name AndroidManifest.xml -exec sed -i '' 's|versionCode="[0-9]*"|versionCode="431"|g' {} \;
        find actionbarsherlock-samples -name AndroidManifest.xml -exec sed -i '' 's|versionName="[0-9.]*"|versionName="4.3.1"|g' {} \;

 5. Make the release!

        mvn clean release:clean
        mvn release:prepare release:perform

 6. Promote the Maven artifact on Sonatype's OSS Nexus install.
 7. Deploy the latest website.

        ./deploy_website.sh
