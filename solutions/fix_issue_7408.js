// Solution for issue #7408
// Generated automatically by Baobot

// This script assumes that the organicmaps/organicmaps repository has a way to backup bookmarks to Google Drive.
// It uses the Google Drive API for this purpose. Please ensure you have the necessary permissions and set up the API.

// Replace these with your actual service account key file path
const SERVICE_ACCOUNT_KEY_PATH = '/path/to/service-account-key.json';

const { google } = require('googleapis');
const os = require('os');
const fs = require('fs');

async function backupBookmarksToGoogleDrive() {
  const [auth, credential] = await Promise.all([
    google.auth.getClient({ key: fs.readFileSync(SERVICE_ACCOUNT_KEY_PATH), scopes: ['https://www.googleapis.com/auth/drive'] }),
    google.drive({ auth })
  ]);

  const folderId = 'YOUR_GOOGLE_DRIVE_FOLDER_ID'; // Replace with your actual folder ID
  const folderPath = 'Bookmarks';

  const metadata = {
    name: folderPath,
    mimeType: 'application/vnd.google-apps.folder',
    parents: [folderId]
  };

  const folder = await auth.drive.files.create({
    resource: metadata,
    fields: 'id'
  });

  const folderId = folder.data.id;

  const bookmarkEntries = [
    {
      title: 'Bookmarks',
      url: 'https://github.com/organicmaps/organicmaps/issues/7408',
      mimeType: 'text/html',
      snippet: '<p>Description of the issue.</p>',
      imageThumbnailUrl: 'https://example.com/bookmark-icon.png',
      alternateLink: 'https://github.com/organicmaps/organicmaps/issues/7408',
      folderId: folderId,
      isConverted: true
    }
  ];

  const batch = auth.drive.newBatchRequest();
  const batchRequests = bookmarkEntries.map((entry) => {
    batch.files.update({ fileId: entry.id, resource: entry });
  });

  batch.execute();
}

backupBookmarksToGoogleDrive();


Please replace the placeholders (`YOUR_GOOGLE_DRIVE_FOLDER_ID` and `SERVICE_ACCOUNT_KEY_PATH`) with actual values. This script assumes the existence of a Google service account key file and a folder ID in your Google Drive. The script creates a new folder in Google Drive and adds a bookmark entry for the GitHub issue.