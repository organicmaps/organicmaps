package app.organicmaps.bookmarks.cloud;

import org.junit.Assert;
import org.junit.Test;

public class NextcloudWebDavClientTest
{
  @Test
  public void testUrlNormalization()
  {
    NextcloudWebDavClient client1 = new NextcloudWebDavClient("https://cloud.example.com", "alice", "token123");
    Assert.assertNotNull(client1);

    NextcloudWebDavClient client2 = new NextcloudWebDavClient("https://cloud.example.com/", "alice", "token123");
    Assert.assertNotNull(client2);

    NextcloudWebDavClient client3 = new NextcloudWebDavClient("https://cloud.example.com/remote.php/dav/files/alice/", "alice", "token123");
    Assert.assertNotNull(client3);
  }
}
