package app.organicmaps.routing;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.ValueAnimator;
import android.content.Context;
import android.content.res.ColorStateList;
import android.text.SpannableStringBuilder;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.animation.AccelerateDecelerateInterpolator;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.ViewCompat;
import androidx.core.widget.TextViewCompat;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import app.organicmaps.sdk.routing.TransitStepInfo;
import app.organicmaps.sdk.routing.TransitStepType;
import app.organicmaps.util.ThemeUtils;
import app.organicmaps.util.Utils;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Renders the per-leg transit route breakdown: one row per leg, showing boarding stop, line badge,
 * stop count + time, and exit stop. The intermediate stops are collapsed until the rider taps the
 * stop-count summary. Walk legs are rendered as a single row.
 *
 * A user-added intermediate point (Add Stop) is rendered as its own numbered badge row between the
 * legs, mirroring the badges on the summary strip so the strip and the expanded details match. It
 * also explains why a continuous walk is split into two rows around the dropped point.
 */
public class TransitDetailsAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder>
{
  private static final int TYPE_WALK = 0;
  private static final int TYPE_RIDE = 1;
  private static final int TYPE_INTERMEDIATE = 2;

  @NonNull
  private final List<TransitStepInfo> mItems = new ArrayList<>();
  @NonNull
  private final Set<Integer> mExpanded = new HashSet<>();

  public void setItems(@NonNull List<TransitStepInfo> items)
  {
    mItems.clear();
    mExpanded.clear();
    // The detail view shows walks, transit rides, and the user's intermediate points (Add Stop).
    // Ruler segments are not meaningful at this level of detail.
    for (TransitStepInfo info : items)
    {
      if (info.getType() == TransitStepType.RULER)
        continue;
      mItems.add(info);
    }
    notifyDataSetChanged();
  }

  @Override
  public int getItemViewType(int position)
  {
    return switch (mItems.get(position).getType())
    {
      case PEDESTRIAN -> TYPE_WALK;
      case INTERMEDIATE_POINT -> TYPE_INTERMEDIATE;
      default -> TYPE_RIDE;
    };
  }

  @NonNull
  @Override
  public RecyclerView.ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType)
  {
    LayoutInflater inflater = LayoutInflater.from(parent.getContext());
    if (viewType == TYPE_WALK)
      return new WalkViewHolder(inflater.inflate(R.layout.item_transit_details_walk, parent, false));
    if (viewType == TYPE_INTERMEDIATE)
      return new IntermediateViewHolder(inflater.inflate(R.layout.item_transit_details_intermediate, parent, false));
    RideViewHolder holder = new RideViewHolder(inflater.inflate(R.layout.item_transit_details_ride, parent, false));
    holder.mSummaryRow.setOnClickListener(v -> {
      int position = holder.getBindingAdapterPosition();
      if (position == RecyclerView.NO_POSITION)
        return;
      boolean expand = !mExpanded.contains(position);
      if (expand)
        mExpanded.add(position);
      else
        mExpanded.remove(position);
      // Animate this row's own height so the whole panel resizes gradually in step, instead of the
      // list snapping shut. A parent-level transition can't tween a nested RecyclerView item collapse.
      holder.setExpanded(expand, true);
    });
    return holder;
  }

  @Override
  public void onBindViewHolder(@NonNull RecyclerView.ViewHolder holder, int position)
  {
    TransitStepInfo info = mItems.get(position);
    if (holder instanceof WalkViewHolder)
      ((WalkViewHolder) holder).bind(info);
    else if (holder instanceof IntermediateViewHolder)
      ((IntermediateViewHolder) holder).bind(info);
    else if (holder instanceof RideViewHolder)
    {
      // Each ride is headed by its boarding stop. When the previous row was already a ride, this is a
      // same-station transfer, so the heading reads "Transfer at <stop>" instead of "Board at <stop>".
      boolean prevIsRide = isRide(position - 1);
      // The exit stop is only drawn when the next row is not a ride: a direct transfer's stop is
      // already shown as the next ride's heading, so repeating it at this leg's foot would duplicate it.
      boolean nextIsRide = isRide(position + 1);
      // The incoming line's color paints the top half of the transfer stub above this leg's bar.
      int incomingColor = prevIsRide ? mItems.get(position - 1).getColor() : info.getColor();
      ((RideViewHolder) holder).bind(info, prevIsRide, nextIsRide, mExpanded.contains(position), incomingColor);
    }
  }

  // A transit ride is any leg that is neither a walk nor a user-added intermediate point.
  private boolean isRide(int position)
  {
    if (position < 0 || position >= mItems.size())
      return false;
    TransitStepType type = mItems.get(position).getType();
    return type != TransitStepType.PEDESTRIAN && type != TransitStepType.INTERMEDIATE_POINT;
  }

  @Override
  public int getItemCount()
  {
    return mItems.size();
  }

  static class WalkViewHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    private final TextView mText;

    WalkViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mText = itemView.findViewById(R.id.walk_text);
    }

    void bind(@NonNull TransitStepInfo info)
    {
      Context ctx = itemView.getContext();
      CharSequence time = Utils.formatRoutingTime(ctx, info.getTimeInSec(), R.dimen.text_size_body_3);
      SpannableStringBuilder text = new SpannableStringBuilder(ctx.getString(R.string.transit_walk_label));
      text.append(" · ").append(time);
      if (info.getDistance() != null && !info.getDistance().isEmpty())
        text.append(" · ").append(info.getDistance()).append(' ').append(info.getDistanceUnits());
      mText.setText(text);
    }
  }

  static class IntermediateViewHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    private final TransitStepView mBadge;
    @NonNull
    private final TextView mLabel;

    IntermediateViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mBadge = itemView.findViewById(R.id.intermediate_badge);
      mLabel = itemView.findViewById(R.id.intermediate_label);
    }

    void bind(@NonNull TransitStepInfo info)
    {
      // The badge shows the waypoint number (index + 1), matching the summary strip.
      mBadge.setTransitStepInfo(info);
      mLabel.setText(itemView.getContext().getString(R.string.transit_waypoint_label, info.getIntermediateIndex() + 1));
    }
  }

  static class RideViewHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    private final TextView mBoardAt;
    @NonNull
    private final TextView mExitAt;
    @NonNull
    private final TextView mSubtitle;
    @NonNull
    private final TextView mIntermediates;
    @NonNull
    private final TransitStepView mBadge;
    @NonNull
    private final View mSummaryRow;
    @NonNull
    private final View mLineBar;
    @NonNull
    private final View mTransferHeaderRow;
    @NonNull
    private final TextView mTransferHeader;
    @NonNull
    private final View mTransferStubTop;
    @NonNull
    private final View mTransferStubBottom;
    @NonNull
    private final View mTransferGap;
    private boolean mHasIntermediates;
    @Nullable
    private ValueAnimator mHeightAnimator;

    RideViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mBoardAt = itemView.findViewById(R.id.board_at);
      mExitAt = itemView.findViewById(R.id.exit_at);
      mSubtitle = itemView.findViewById(R.id.ride_subtitle);
      mIntermediates = itemView.findViewById(R.id.intermediate_stops);
      mBadge = itemView.findViewById(R.id.line_badge);
      mSummaryRow = itemView.findViewById(R.id.ride_summary_row);
      mLineBar = itemView.findViewById(R.id.line_bar);
      mTransferHeaderRow = itemView.findViewById(R.id.transfer_header_row);
      mTransferHeader = itemView.findViewById(R.id.transfer_header);
      mTransferStubTop = itemView.findViewById(R.id.transfer_stub_top);
      mTransferStubBottom = itemView.findViewById(R.id.transfer_stub_bottom);
      mTransferGap = itemView.findViewById(R.id.transfer_gap);
    }

    void bind(@NonNull TransitStepInfo info, boolean isTransfer, boolean nextIsRide, boolean expanded,
              int incomingColor)
    {
      Context ctx = itemView.getContext();
      mBadge.setTransitStepInfo(info);
      int lineColor = info.getColor();
      // Each colored segment is a rounded pill. The bar's top is rounded unless it continues up out of a
      // transfer stub (isTransfer); its bottom is rounded unless it continues down into the next leg's
      // stub (nextIsRide). Where a stub and the bar share a color they meet square, so each run reads as
      // one pill rounded only at its two ends.
      mLineBar.setBackgroundResource(lineBarDrawable(!isTransfer, !nextIsRide));
      ViewCompat.setBackgroundTintList(mLineBar, ColorStateList.valueOf(lineColor));

      String startStop = info.getStartStopName();
      String endStop = info.getEndStopName();
      // At a same-station transfer the shared stop is drawn once, centered beside a small break in the
      // colored line (incoming color above, outgoing below). The in-bar boarding heading is then hidden
      // and the bar joins straight onto the stub below the break.
      String startStopText = startStop == null ? "" : startStop;
      mTransferHeaderRow.setVisibility(isTransfer ? View.VISIBLE : View.GONE);
      mBoardAt.setVisibility(isTransfer ? View.GONE : View.VISIBLE);
      if (isTransfer)
      {
        mTransferHeader.setText(ctx.getString(R.string.transit_transfer_at, startStopText));
        // The stub is the pill end at the break: incoming run rounded at its foot, outgoing at its head.
        mTransferStubTop.setBackgroundResource(R.drawable.transit_line_bar_bottom);
        mTransferStubBottom.setBackgroundResource(R.drawable.transit_line_bar_top);
        ViewCompat.setBackgroundTintList(mTransferStubTop, ColorStateList.valueOf(incomingColor));
        ViewCompat.setBackgroundTintList(mTransferStubBottom, ColorStateList.valueOf(lineColor));
        ViewGroup.LayoutParams gapLp = mTransferGap.getLayoutParams();
        gapLp.height = Math.round(0.3f * mTransferHeader.getLineHeight());
        mTransferGap.setLayoutParams(gapLp);
      }
      else
        mBoardAt.setText(ctx.getString(R.string.transit_board_at, startStopText));

      // On a transfer the bar continues straight from the stub, so drop its usual top offset; the badge
      // row likewise drops its top margin because the transfer header above it already spaces the rhythm.
      int marginHalf = ctx.getResources().getDimensionPixelSize(R.dimen.margin_half);
      ViewGroup.MarginLayoutParams barLp = (ViewGroup.MarginLayoutParams) mLineBar.getLayoutParams();
      barLp.topMargin = isTransfer ? 0 : marginHalf;
      mLineBar.setLayoutParams(barLp);
      ViewGroup.MarginLayoutParams summaryLp = (ViewGroup.MarginLayoutParams) mSummaryRow.getLayoutParams();
      summaryLp.topMargin = isTransfer ? 0 : marginHalf;
      mSummaryRow.setLayoutParams(summaryLp);

      if (nextIsRide)
      {
        mExitAt.setVisibility(View.GONE);
      }
      else
      {
        // The exit heading only shows when the next leg is not a train, so the rider always leaves the
        // train here: "Exit at <stop>". A same-station line change is the other branch (exit hidden, the
        // next leg draws the centered "Transfer at <stop>" header instead).
        mExitAt.setVisibility(View.VISIBLE);
        mExitAt.setText(ctx.getString(R.string.transit_exit_at, endStop == null ? "" : endStop));
      }

      CharSequence time = Utils.formatRoutingTime(ctx, info.getTimeInSec(), R.dimen.text_size_body_3);
      int stopCount = info.getStopCount();
      SpannableStringBuilder subtitle = new SpannableStringBuilder();
      if (stopCount > 0)
      {
        subtitle.append(ctx.getResources().getQuantityString(R.plurals.transit_stops_count, stopCount, stopCount));
        subtitle.append(" · ");
      }
      subtitle.append(time);
      mSubtitle.setText(subtitle);

      String[] intermediates = info.getIntermediateStopNames();
      mHasIntermediates = intermediates != null && intermediates.length > 0;
      // Only a leg with intermediate stops is an expandable disclosure; others get no chevron or ripple.
      mSummaryRow.setClickable(mHasIntermediates);
      // Text is always set (even while hidden) so the collapse/expand animation can measure it.
      if (mHasIntermediates)
        // One stop per line so the intermediate stops read as a column between board and exit.
        mIntermediates.setText(String.join("\n", intermediates));
      setExpanded(expanded, false);
    }

    // Reflects the expanded state on the chevron and the intermediate stops. When animate is true the
    // stops slide open/closed by tweening their own height, which drives the whole panel to resize with
    // them; otherwise (a fresh bind) the state is applied instantly.
    void setExpanded(boolean expanded, boolean animate)
    {
      Context ctx = itemView.getContext();
      int chevron = !mHasIntermediates ? 0 : (expanded ? R.drawable.ic_expand_less : R.drawable.ic_expand_more);
      mSubtitle.setCompoundDrawablesRelativeWithIntrinsicBounds(0, 0, chevron, 0);
      TextViewCompat.setCompoundDrawableTintList(mSubtitle,
                                                 ColorStateList.valueOf(ThemeUtils.getColor(ctx, R.attr.secondary)));

      if (mHeightAnimator != null)
      {
        mHeightAnimator.cancel();
        mHeightAnimator = null;
      }

      if (!mHasIntermediates || !animate)
      {
        mIntermediates.setVisibility(mHasIntermediates && expanded ? View.VISIBLE : View.GONE);
        setViewHeight(mIntermediates, ViewGroup.LayoutParams.WRAP_CONTENT);
        return;
      }

      mIntermediates.setVisibility(View.VISIBLE);
      int from;
      int to;
      if (expanded)
      {
        int width = ((View) mIntermediates.getParent()).getWidth();
        mIntermediates.measure(View.MeasureSpec.makeMeasureSpec(width, View.MeasureSpec.EXACTLY),
                               View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED));
        from = 0;
        to = mIntermediates.getMeasuredHeight();
      }
      else
      {
        from = mIntermediates.getHeight();
        to = 0;
      }

      ValueAnimator animator = ValueAnimator.ofInt(from, to);
      animator.setDuration(150);
      animator.setInterpolator(new AccelerateDecelerateInterpolator());
      animator.addUpdateListener(a -> setViewHeight(mIntermediates, (int) a.getAnimatedValue()));
      animator.addListener(new AnimatorListenerAdapter() {
        @Override
        public void onAnimationEnd(Animator animation)
        {
          if (!expanded)
            mIntermediates.setVisibility(View.GONE);
          setViewHeight(mIntermediates, ViewGroup.LayoutParams.WRAP_CONTENT);
          mHeightAnimator = null;
        }
      });
      mHeightAnimator = animator;
      animator.start();
    }

    private static void setViewHeight(@NonNull View view, int height)
    {
      ViewGroup.LayoutParams lp = view.getLayoutParams();
      lp.height = height;
      view.setLayoutParams(lp);
    }
  }

  private static int lineBarDrawable(boolean roundTop, boolean roundBottom)
  {
    if (roundTop && roundBottom)
      return R.drawable.transit_line_bar_round;
    if (roundTop)
      return R.drawable.transit_line_bar_top;
    if (roundBottom)
      return R.drawable.transit_line_bar_bottom;
    return R.drawable.transit_line_bar;
  }
}
