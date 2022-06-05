package app.organicmaps.api.example;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.widget.Toast;

import app.organicmaps.api.example.databinding.ActivityMainBinding;

import java.util.Locale;

public class MainActivity extends Activity
{
  private ActivityMainBinding binding;

  private static final int REQ_CODE_LOCATION = 1;

  @Override
  protected void onCreate(Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);

    binding = ActivityMainBinding.inflate(getLayoutInflater());
    setContentView(binding.getRoot());

    binding.locationTest.setOnClickListener(view -> {
      Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse("om://location"));
      startActivityForResult(intent, REQ_CODE_LOCATION);
    });
  }

  protected void onActivityResult(int requestCode,
                                  int resultCode,
                                  Intent data)
  {
    switch (requestCode)
    {
    case REQ_CODE_LOCATION:
      if (resultCode == RESULT_CANCELED)
      {
        Toast.makeText(this, "Cancelled", Toast.LENGTH_SHORT).show();
        return;
      }
      else if (resultCode != RESULT_OK)
      {
        throw new AssertionError("Unsupported resultCode: " + resultCode);
      }

      double lat = data.getDoubleExtra("lat", 0.0);
      double lon = data.getDoubleExtra("lon", 0.0);
      String message = String.format(Locale.ENGLISH, "Result: lat=%.4f lon=%.4f", lat, lon);
      Toast.makeText(this, message, Toast.LENGTH_SHORT).show();
      break;
    default:
      super.onActivityResult(requestCode, resultCode, data);
    }
  }
}