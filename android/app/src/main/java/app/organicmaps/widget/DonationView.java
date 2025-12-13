package app.organicmaps.widget;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.util.AttributeSet;
import android.view.HapticFeedbackConstants;
import android.view.LayoutInflater;
import android.view.View;
import android.view.animation.AccelerateDecelerateInterpolator;
import android.widget.FrameLayout;
import android.widget.Toast;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.BuildConfig;
import app.organicmaps.R;
import app.organicmaps.sdk.Framework;

public class DonationView extends FrameLayout
{
  @Nullable
  private Runnable mOnDonateClickListener;

  private final Handler mHandler = new Handler(Looper.getMainLooper());

  private View mCardRoot;
  private View mHighlightOverlay;
  private View mActionButton;

  private boolean mAnimating;

  private static final float PULSE_SCALE = 1.03f;
  private static final float PRESS_SCALE = 0.97f;
  private static final long PULSE_MS = 140;
  private static final long PRESS_MS = 90;
  private static final long RELEASE_MS = 140;

  private final AccelerateDecelerateInterpolator mInterpolator = new AccelerateDecelerateInterpolator();

  public DonationView(@NonNull Context context)
  {
    this(context, null);
  }
  public DonationView(@NonNull Context context, @Nullable AttributeSet attrs)
  {
    this(context, attrs, 0);
  }

  public DonationView(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs, defStyleAttr);
    init(context);
  }

  private void init(@NonNull Context context)
  {
    LayoutInflater.from(context).inflate(R.layout.view_donation, this, true);
  }

  @Override
  protected void onFinishInflate()
  {
    super.onFinishInflate();

    View descriptionText = findViewById(R.id.donation_description);
    mActionButton = findViewById(R.id.donation_actionButton);
    mHighlightOverlay = findViewById(R.id.donation_highlightOverlay);
    mCardRoot = findViewById(R.id.donationRoot); // root MaterialCardView

    mActionButton.setOnClickListener(v -> runDonateAnimationAndThen(this::runDonate));

    if (BuildConfig.DEBUG)
    {
      descriptionText.setOnLongClickListener(v -> {
        Framework.nativeResetDonations();
        Toast.makeText(getContext(), "Donations statistics reset", Toast.LENGTH_SHORT).show();
        return true;
      });
    }
  }

  public void setOnDonateClickListener(@Nullable Runnable listener)
  {
    mOnDonateClickListener = listener;
  }

  private void runDonate()
  {
    if (mOnDonateClickListener != null)
      mOnDonateClickListener.run();
  }

  private void runDonateAnimationAndThen(@NonNull Runnable after)
  {
    if (mAnimating || mOnDonateClickListener == null)
      return;

    final View target = (mCardRoot != null) ? mCardRoot : this;

    if (target.getWidth() == 0 || target.getHeight() == 0)
    {
      target.post(() -> runDonateAnimationAndThen(after));
      return;
    }

    mAnimating = true;

    target.animate().cancel();
    if (mHighlightOverlay != null)
      mHighlightOverlay.animate().cancel();

    if (mHighlightOverlay != null)
      mHighlightOverlay.setAlpha(0f);

    target.setPivotX(target.getWidth() * 0.5f);
    target.setPivotY(target.getHeight() * 0.5f);

    if (mActionButton != null)
    {
      mActionButton.drawableHotspotChanged(mActionButton.getWidth() * 0.2f, mActionButton.getHeight() * 0.2f);
      mActionButton.setPressed(true);
      mActionButton.postDelayed(() -> mActionButton.setPressed(false), 80);
    }

    target.animate()
        .scaleX(PULSE_SCALE)
        .scaleY(PULSE_SCALE)
        .setDuration(PULSE_MS)
        .setInterpolator(mInterpolator)
        .withEndAction(() -> {
          target.animate()
              .scaleX(PRESS_SCALE)
              .scaleY(PRESS_SCALE)
              .setDuration(PRESS_MS)
              .setInterpolator(mInterpolator)
              .withStartAction(() -> {
                target.performHapticFeedback(HapticFeedbackConstants.CONTEXT_CLICK);
                if (mHighlightOverlay != null)
                  mHighlightOverlay.animate().alpha(1f).setDuration(PRESS_MS).start();
              })
              .withEndAction(() -> {
                after.run();
                target.animate().scaleX(1f).scaleY(1f).setDuration(RELEASE_MS).start();
                if (mHighlightOverlay != null)
                  mHighlightOverlay.animate().alpha(0f).setDuration(RELEASE_MS).start();

                mAnimating = false;
              })
              .start();
        })
        .start();
  }

  @Override
  protected void onDetachedFromWindow()
  {
    super.onDetachedFromWindow();
    mHandler.removeCallbacksAndMessages(null);
  }
}
