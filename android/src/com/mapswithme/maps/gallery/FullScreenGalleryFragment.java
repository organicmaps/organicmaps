package com.mapswithme.maps.gallery;

import android.os.Bundle;
import androidx.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;

import com.bumptech.glide.Glide;
import com.bumptech.glide.load.resource.drawable.GlideDrawable;
import com.bumptech.glide.request.RequestListener;
import com.bumptech.glide.request.target.Target;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.util.Utils;

public class FullScreenGalleryFragment extends BaseMwmFragment
{
  static final String ARGUMENT_IMAGE = "argument_image";

  @Nullable
  private Image mImage;

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_fullscreen_image, container, false);
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    readArguments();

    if (mImage != null)
    {
      ImageView imageView = (ImageView) view.findViewById(R.id.iv__image);
      final View progress = view.findViewById(R.id.pb__loading_image);
      Glide.with(view.getContext())
           .load(mImage.getUrl())
           .listener(new RequestListener<String, GlideDrawable>()
           {
             @Override
             public boolean onException(Exception e, String model, Target<GlideDrawable> target,
                                        boolean isFirstResource)
             {
               if (isVisible())
               {
                 progress.setVisibility(View.GONE);
                 Utils.showSnackbar(getViewOrThrow(), getString(R.string.download_failed));
               }
               return false;
             }

             @Override
             public boolean onResourceReady(GlideDrawable resource, String model,
                                            Target<GlideDrawable> target, boolean isFromMemoryCache,
                                            boolean isFirstResource)
             {
               if (isVisible())
                progress.setVisibility(View.GONE);
               return false;
             }
           })
           .into(imageView);
    }
  }

  private void readArguments()
  {
    Bundle args = getArguments();
    if (args != null)
      mImage = args.getParcelable(ARGUMENT_IMAGE);
  }
}
