package app.organicmaps.search;

import static org.junit.Assert.*;

import java.util.List;
import org.junit.Test;

public class ContactResolutionPolicyTest
{
  @Test
  public void smallMovesAndZoomsPreserveMisses()
  {
    ContactResolutionPolicy policy = new ContactResolutionPolicy();
    assertTrue(policy.updateArea("Canada BC", 1, 0, 0, 4, 4));
    policy.recordMiss("address", 100);
    assertFalse(policy.updateArea("Canada BC", 1, 0.5, 0, 4.5, 4));
    assertFalse(policy.updateArea("Canada BC", 1, 1, 1, 3, 3));
    assertFalse(policy.shouldRetry("address", 200));
    assertTrue(policy.shouldRetry("address", 300100));
  }

  @Test
  public void accumulatedMovementUsesOriginalArea()
  {
    ContactResolutionPolicy policy = new ContactResolutionPolicy();
    policy.updateArea("Canada BC", 1, 0, 0, 4, 4);
    assertFalse(policy.updateArea("Canada BC", 1, 0.75, 0, 4.75, 4));
    assertTrue(policy.updateArea("Canada BC", 1, 1.5, 0, 5.5, 4));
  }

  @Test
  public void differentAreaOrMapVersionAllowsRetry()
  {
    ContactResolutionPolicy policy = new ContactResolutionPolicy();
    policy.updateArea("Canada BC", 1, 0, 0, 4, 4);
    policy.recordMiss("address", 0);
    assertTrue(policy.updateArea("Canada BC", 2, 0, 0, 4, 4));
    assertTrue(policy.shouldRetry("address", 1));
    assertTrue(policy.updateArea("Canada MB", 2, 0, 0, 4, 4));
    assertTrue(policy.updateArea("Canada MB", 2, -4, -4, 8, 8));
  }

  @Test
  public void resetAndSuccessfulResolutionForgetMisses()
  {
    ContactResolutionPolicy policy = new ContactResolutionPolicy();
    policy.updateArea("region", 1, 0, 0, 4, 4);
    policy.recordMiss("address", 0);
    policy.forget("address");
    assertTrue(policy.shouldRetry("address", 1));
    policy.recordMiss("address", 1);
    policy.clear();
    assertTrue(policy.shouldRetry("address", 2));
    assertTrue(policy.updateArea("region", 1, 0, 0, 4, 4));
  }

  @Test
  public void localityMatchIsAPriorityHint()
  {
    List<String> map = List.of("canada", "british", "columbia", "vancouver");
    assertTrue(ContactResolutionPolicy.regionScore(List.of("vancouver", "british", "columbia"), map)
               > ContactResolutionPolicy.regionScore(List.of("surrey", "british", "columbia"), map));
    assertEquals(0, ContactResolutionPolicy.regionScore(List.of(), map));
  }
}
