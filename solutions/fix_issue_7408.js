// Solution for issue #7408
// Generated automatically by Baobot

const DriveApi = require('@google/googleapis').Drive;

async function backupBookmarksToGoogleDrive() {
    const drive = DriveApi.drive({version: 'v3'});
    const credentials = require('./credentials.json'); // Assuming this file contains OAuth2 credentials
    const auth = new google.auth.GoogleAuth({
        credentials: credentials,
    });

    const client = auth.getClient();
    drive.setRequestMirror({
        headers: {
            authorization: `Bearer ${client.token.json.access_token}`,
        },
    });

    const fileMetadata = {
        name: 'organicmaps_bookmarks.json',
        mimeType: 'application/json',
    };

    const contents = require('./bookmarks.json'); // Assuming this file contains the bookmarks data

    const file = await drive.files.create({
        auth: client,
        resource: fileMetadata,
        media: {data: Buffer.from(JSON.stringify(contents)).toString('base64')},
        fields: 'id',
    });

    console.log(`Bookmarks backed up to Google Drive. File ID: ${file.id}`);
}

backupBookmarksToGoogleDrive();


This script assumes you have a file named `credentials.json` with OAuth2 credentials and a file named `bookmarks.json` containing the bookmarks data. You need to adjust paths and credentials accordingly.