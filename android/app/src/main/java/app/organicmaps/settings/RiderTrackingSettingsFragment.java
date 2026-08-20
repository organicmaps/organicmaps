package app.organicmaps.settings;

import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.text.InputType;
import android.widget.EditText;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.EditTextPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.PreferenceScreen;
import androidx.preference.SwitchPreferenceCompat;

import app.organicmaps.R;
import app.organicmaps.location.RiderTrackingManager;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import java.util.Set;

public class RiderTrackingSettingsFragment extends BaseXmlSettingsFragment
{
  private RiderTrackingManager mRiderManager;

  @Override
  protected int getXmlResources()
  {
    return 0; // PreferenceScreen built programmatically
  }

  @Override
  public void onCreatePreferences(Bundle savedInstanceState, String rootKey)
  {
    Context context = requireContext();
    mRiderManager = RiderTrackingManager.get(context);

    PreferenceScreen screen = getPreferenceManager().createPreferenceScreen(context);

    // --- SECTION 1: MY RIDER PROFILE ---
    PreferenceCategory profileCategory = new PreferenceCategory(context);
    profileCategory.setTitle("My Rider Profile");
    screen.addPreference(profileCategory);

    // My display name — shown on friends' maps as the label above the dot
    Preference myNamePref = new Preference(context);
    myNamePref.setTitle("My Display Name");
    myNamePref.setSummary(myNameSummary());
    myNamePref.setOnPreferenceClickListener(preference -> {
      showEditNameDialog(context, myNamePref);
      return true;
    });
    profileCategory.addPreference(myNamePref);

    // My Rider Code Display & Copy/Share
    Preference myCodePref = new Preference(context);
    myCodePref.setKey("rider_my_code_pref");
    myCodePref.setTitle("My Rider Code");
    myCodePref.setSummary(mRiderManager.getMyRiderCode() + "  (Tap to Copy/Share)");
    myCodePref.setIcon(R.drawable.ic_share);
    myCodePref.setOnPreferenceClickListener(preference -> {
      String code = mRiderManager.getMyRiderCode();
      ClipboardManager clipboard = (ClipboardManager) context.getSystemService(Context.CLIPBOARD_SERVICE);
      if (clipboard != null)
      {
        clipboard.setPrimaryClip(ClipData.newPlainText("Rider Code", code));
        Toast.makeText(context, "Rider Code " + code + " copied to clipboard!", Toast.LENGTH_SHORT).show();
      }

      Intent shareIntent = new Intent(Intent.ACTION_SEND);
      shareIntent.setType("text/plain");
      shareIntent.putExtra(Intent.EXTRA_TEXT, "Track my ride on Organic Maps using my Rider Code: " + code);
      startActivity(Intent.createChooser(shareIntent, "Share Rider Code"));
      return true;
    });
    profileCategory.addPreference(myCodePref);

    // Toggle Sharing
    SwitchPreferenceCompat sharingPref = new SwitchPreferenceCompat(context);
    sharingPref.setKey("rider_sharing_switch");
    sharingPref.setTitle("Share My Live Location");
    sharingPref.setSummary("Broadcast current coordinates to cloud VSI server while riding");
    sharingPref.setChecked(mRiderManager.isSharingEnabled());
    sharingPref.setOnPreferenceChangeListener((preference, newValue) -> {
      boolean enabled = (Boolean) newValue;
      mRiderManager.setSharingEnabled(enabled);
      Toast.makeText(context, enabled ? "Live location sharing enabled" : "Location sharing disabled", Toast.LENGTH_SHORT).show();
      return true;
    });
    profileCategory.addPreference(sharingPref);

    // Show myself on the map — round-trips through the server so the marker
    // reflects what other riders actually see. Handy for verifying accuracy.
    SwitchPreferenceCompat showSelfPref = new SwitchPreferenceCompat(context);
    showSelfPref.setKey("rider_show_myself_switch");
    showSelfPref.setTitle("Show Myself on Map");
    showSelfPref.setSummary("Draw your own dot from server data (for testing that others see you correctly)");
    showSelfPref.setChecked(mRiderManager.isShowMyselfEnabled());
    showSelfPref.setOnPreferenceChangeListener((preference, newValue) -> {
      boolean enabled = (Boolean) newValue;
      mRiderManager.setShowMyselfEnabled(enabled);
      Toast.makeText(context, enabled ? "Self marker enabled" : "Self marker disabled", Toast.LENGTH_SHORT).show();
      return true;
    });
    profileCategory.addPreference(showSelfPref);

    // --- SECTION 2: CLOUD VSI SERVER SETTINGS ---
    PreferenceCategory serverCategory = new PreferenceCategory(context);
    serverCategory.setTitle("Cloud VSI Server Settings");
    screen.addPreference(serverCategory);

    Preference serverPref = new Preference(context);
    serverPref.setTitle("Server URL / IP");
    serverPref.setSummary(mRiderManager.getServerUrl());
    serverPref.setOnPreferenceClickListener(preference -> {
      showEditServerDialog(context, serverPref);
      return true;
    });
    serverCategory.addPreference(serverPref);

    Preference userPref = new Preference(context);
    userPref.setTitle("Server Username");
    userPref.setSummary(serverUsernameSummary());
    userPref.setOnPreferenceClickListener(preference -> {
      showEditUsernameDialog(context, userPref);
      return true;
    });
    serverCategory.addPreference(userPref);

    Preference passPref = new Preference(context);
    passPref.setTitle("Server Password");
    passPref.setSummary(serverPasswordSummary());
    passPref.setOnPreferenceClickListener(preference -> {
      showEditPasswordDialog(context, passPref);
      return true;
    });
    serverCategory.addPreference(passPref);

    // --- SECTION 3: FRIEND RIDER CODES ---
    PreferenceCategory friendsCategory = new PreferenceCategory(context);
    friendsCategory.setTitle("Friend Rider Codes");
    screen.addPreference(friendsCategory);

    Preference addFriendPref = new Preference(context);
    addFriendPref.setTitle("Add Friend Rider Code");
    addFriendPref.setSummary("Enter a friend's alphanumeric code to see them on the map");
    addFriendPref.setIcon(R.drawable.ic_add_list);
    addFriendPref.setOnPreferenceClickListener(preference -> {
      showAddFriendDialog(context, friendsCategory);
      return true;
    });
    friendsCategory.addPreference(addFriendPref);

    // List existing friend codes
    refreshFriendPreferences(context, friendsCategory);

    setPreferenceScreen(screen);
  }

  private void showEditServerDialog(Context context, Preference serverPref)
  {
    final EditText input = new EditText(context);
    input.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_URI);
    input.setText(mRiderManager.getServerUrl());

    new MaterialAlertDialogBuilder(context)
        .setTitle("Cloud VSI Server URL")
        .setMessage("Enter the IP / URL and port of your rider server (e.g. http://192.168.1.100:8080)")
        .setView(input)
        .setPositiveButton("Save", (dialog, which) -> {
          String url = input.getText().toString().trim();
          if (!url.isEmpty())
          {
            mRiderManager.setServerUrl(url);
            serverPref.setSummary(url);
            Toast.makeText(context, "Server URL updated", Toast.LENGTH_SHORT).show();
          }
        })
        .setNegativeButton("Cancel", null)
        .show();
  }

  private void showEditUsernameDialog(Context context, Preference pref)
  {
    final EditText input = new EditText(context);
    input.setInputType(InputType.TYPE_CLASS_TEXT);
    input.setText(mRiderManager.getServerUsername());

    new MaterialAlertDialogBuilder(context)
        .setTitle("Server Username")
        .setMessage("HTTP Basic Auth username for the tracking server. Leave empty to disable auth.")
        .setView(input)
        .setPositiveButton("Save", (dialog, which) -> {
          String user = input.getText().toString().trim();
          mRiderManager.setServerUsername(user);
          pref.setSummary(serverUsernameSummary());
          Toast.makeText(context, "Username updated", Toast.LENGTH_SHORT).show();
        })
        .setNegativeButton("Cancel", null)
        .show();
  }

  private void showEditPasswordDialog(Context context, Preference pref)
  {
    final EditText input = new EditText(context);
    input.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD);
    input.setText(mRiderManager.getServerPassword());

    new MaterialAlertDialogBuilder(context)
        .setTitle("Server Password")
        .setMessage("HTTP Basic Auth password for the tracking server.")
        .setView(input)
        .setPositiveButton("Save", (dialog, which) -> {
          String pass = input.getText().toString();
          mRiderManager.setServerPassword(pass);
          pref.setSummary(serverPasswordSummary());
          Toast.makeText(context, "Password updated", Toast.LENGTH_SHORT).show();
        })
        .setNegativeButton("Cancel", null)
        .show();
  }

  private void showEditNameDialog(Context context, Preference pref)
  {
    final EditText input = new EditText(context);
    input.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_CAP_WORDS);
    input.setText(mRiderManager.getMyName());
    input.setHint("e.g. Faris");

    new MaterialAlertDialogBuilder(context)
        .setTitle("My Display Name")
        .setMessage("Name shown on friends' maps above your dot. Falls back to rider code if empty.")
        .setView(input)
        .setPositiveButton("Save", (dialog, which) -> {
          String name = input.getText().toString().trim();
          mRiderManager.setMyName(name);
          pref.setSummary(myNameSummary());
          Toast.makeText(context, "Display name updated", Toast.LENGTH_SHORT).show();
        })
        .setNegativeButton("Cancel", null)
        .show();
  }

  private String myNameSummary()
  {
    String n = mRiderManager.getMyName();
    return n.isEmpty() ? "Not set (rider code will be shown)" : n;
  }

  private String serverUsernameSummary()
  {
    String u = mRiderManager.getServerUsername();
    return u.isEmpty() ? "Not set (auth disabled)" : u;
  }

  private String serverPasswordSummary()
  {
    return mRiderManager.getServerPassword().isEmpty() ? "Not set (auth disabled)" : "••••••••";
  }

  private void showAddFriendDialog(Context context, PreferenceCategory friendsCategory)
  {
    final EditText input = new EditText(context);
    input.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_CAP_CHARACTERS);
    input.setHint("e.g. RIDER-X9F2K");

    new MaterialAlertDialogBuilder(context)
        .setTitle("Add Friend Rider Code")
        .setMessage("Enter the alphanumeric code shared by your friend")
        .setView(input)
        .setPositiveButton("Add", (dialog, which) -> {
          String code = input.getText().toString().trim();
          if (!code.isEmpty())
          {
            mRiderManager.addFriendCode(code);
            refreshFriendPreferences(context, friendsCategory);
            Toast.makeText(context, "Friend " + code.toUpperCase() + " added to map tracking", Toast.LENGTH_SHORT).show();
          }
        })
        .setNegativeButton("Cancel", null)
        .show();
  }

  private void refreshFriendPreferences(Context context, PreferenceCategory friendsCategory)
  {
    // Remove old friend code items (keep item at index 0 which is "Add Friend")
    while (friendsCategory.getPreferenceCount() > 1)
    {
      friendsCategory.removePreference(friendsCategory.getPreference(1));
    }

    Set<String> friendCodes = mRiderManager.getFriendCodes();
    if (friendCodes.isEmpty())
    {
      Preference emptyPref = new Preference(context);
      emptyPref.setTitle("No friends added yet");
      emptyPref.setSummary("Tap above to add a friend's rider code");
      emptyPref.setEnabled(false);
      friendsCategory.addPreference(emptyPref);
    }
    else
    {
      for (String code : friendCodes)
      {
        Preference pref = new Preference(context);
        pref.setTitle(code);
        pref.setSummary("Tap to remove friend");
        pref.setIcon(R.drawable.ic_cancel);
        pref.setOnPreferenceClickListener(p -> {
          new MaterialAlertDialogBuilder(context)
              .setTitle("Remove Friend Code?")
              .setMessage("Stop tracking location for " + code + "?")
              .setPositiveButton("Remove", (dialog, which) -> {
                mRiderManager.removeFriendCode(code);
                refreshFriendPreferences(context, friendsCategory);
                Toast.makeText(context, "Removed " + code, Toast.LENGTH_SHORT).show();
              })
              .setNegativeButton("Cancel", null)
              .show();
          return true;
        });
        friendsCategory.addPreference(pref);
      }
    }
  }
}
