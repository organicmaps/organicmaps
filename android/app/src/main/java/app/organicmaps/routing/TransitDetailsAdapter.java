package app.organicmaps.routing;

import android.content.Context;
import android.content.res.ColorStateList;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
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
 */
public class TransitDetailsAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder>
{
  private static final int TYPE_WALK = 0;
  private static final int TYPE_RIDE = 1;

  @NonNull
  private final List<TransitStepInfo> mItems = new ArrayList<>();
  @NonNull
  private final Set<Integer> mExpanded = new HashSet<>();
  // Runs before the list mutates so the host can start a smooth bottom-sheet resize transition.
  @Nullable
  private Runnable mOnBeforeToggle;

  public void setOnBeforeToggleListener(@Nullable Runnable listener)
  {
    mOnBeforeToggle = listener;
  }

  public void setItems(@NonNull List<TransitStepInfo> items)
  {
    mItems.clear();
    mExpanded.clear();
    // The detail view only shows walks and transit rides. Intermediate-point markers and ruler
    // segments are not meaningful at this level of detail.
    for (TransitStepInfo info : items)
    {
      TransitStepType type = info.getType();
      if (type == TransitStepType.INTERMEDIATE_POINT || type == TransitStepType.RULER)
        continue;
      mItems.add(info);
    }
    notifyDataSetChanged();
  }

  @Override
  public int getItemViewType(int position)
  {
    return mItems.get(position).getType() == TransitStepType.PEDESTRIAN ? TYPE_WALK : TYPE_RIDE;
  }

  @NonNull
  @Override
  public RecyclerView.ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType)
  {
    LayoutInflater inflater = LayoutInflater.from(parent.getContext());
    if (viewType == TYPE_WALK)
      return new WalkViewHolder(inflater.inflate(R.layout.item_transit_details_walk, parent, false));
    RideViewHolder holder = new RideViewHolder(inflater.inflate(R.layout.item_transit_details_ride, parent, false));
    holder.mSummaryRow.setOnClickListener(v -> toggleExpanded(holder.getBindingAdapterPosition()));
    return holder;
  }

  private void toggleExpanded(int position)
  {
    if (position == RecyclerView.NO_POSITION)
      return;
    if (!mExpanded.remove(position))
      mExpanded.add(position);
    if (mOnBeforeToggle != null)
      mOnBeforeToggle.run();
    notifyItemChanged(position);
  }

  @Override
  public void onBindViewHolder(@NonNull RecyclerView.ViewHolder holder, int position)
  {
    TransitStepInfo info = mItems.get(position);
    if (holder instanceof WalkViewHolder)
      ((WalkViewHolder) holder).bind(info);
    else if (holder instanceof RideViewHolder)
    {
      // Each ride is headed by its boarding stop. When the previous row was already a ride, this is a
      // same-station transfer, so the heading reads "Transfer at <stop>" instead of "Board at <stop>".
      boolean prevIsRide = position > 0 && mItems.get(position - 1).getType() != TransitStepType.PEDESTRIAN;
      // The exit stop is only drawn when the next row is not a ride: a direct transfer's stop is
      // already shown as the next ride's heading, so repeating it at this leg's foot would duplicate it.
      boolean nextIsRide =
          position + 1 < mItems.size() && mItems.get(position + 1).getType() != TransitStepType.PEDESTRIAN;
      ((RideViewHolder) holder).bind(info, prevIsRide, nextIsRide, isLastRide(position), mExpanded.contains(position));
    }
  }

  private boolean isLastRide(int position)
  {
    for (int i = position + 1; i < mItems.size(); i++)
    {
      if (mItems.get(i).getType() != TransitStepType.PEDESTRIAN)
        return false;
    }
    return true;
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
      StringBuilder text = new StringBuilder(ctx.getString(R.string.transit_walk_label));
      text.append(" · ").append(time);
      if (info.getDistance() != null && !info.getDistance().isEmpty())
        text.append(" · ").append(info.getDistance()).append(' ').append(info.getDistanceUnits());
      mText.setText(text);
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

    RideViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mBoardAt = itemView.findViewById(R.id.board_at);
      mExitAt = itemView.findViewById(R.id.exit_at);
      mSubtitle = itemView.findViewById(R.id.ride_subtitle);
      mIntermediates = itemView.findViewById(R.id.intermediate_stops);
      mBadge = itemView.findViewById(R.id.line_badge);
      mSummaryRow = itemView.findViewById(R.id.ride_summary_row);
    }

    void bind(@NonNull TransitStepInfo info, boolean isTransfer, boolean nextIsRide, boolean isLastRide,
              boolean expanded)
    {
      Context ctx = itemView.getContext();
      mBadge.setTransitStepInfo(info);

      String startStop = info.getStartStopName();
      String endStop = info.getEndStopName();
      int startLabelRes = isTransfer ? R.string.transit_transfer_at : R.string.transit_board_at;
      mBoardAt.setText(ctx.getString(startLabelRes, startStop == null ? "" : startStop));

      if (nextIsRide)
      {
        mExitAt.setVisibility(View.GONE);
      }
      else
      {
        mExitAt.setVisibility(View.VISIBLE);
        int endLabelRes = isLastRide ? R.string.transit_exit_at : R.string.transit_transfer_at;
        mExitAt.setText(ctx.getString(endLabelRes, endStop == null ? "" : endStop));
      }

      CharSequence time = Utils.formatRoutingTime(ctx, info.getTimeInSec(), R.dimen.text_size_body_3);
      int stopCount = info.getStopCount();
      StringBuilder subtitle = new StringBuilder();
      if (stopCount > 0)
      {
        subtitle.append(ctx.getResources().getQuantityString(R.plurals.transit_stops_count, stopCount, stopCount));
        subtitle.append(" · ");
      }
      subtitle.append(time);
      mSubtitle.setText(subtitle);

      String[] intermediates = info.getIntermediateStopNames();
      boolean hasIntermediates = intermediates != null && intermediates.length > 0;
      // Only a leg with intermediate stops is an expandable disclosure; others get no chevron or ripple.
      mSummaryRow.setClickable(hasIntermediates);
      int chevron = !hasIntermediates ? 0 : (expanded ? R.drawable.ic_expand_less : R.drawable.ic_expand_more);
      mSubtitle.setCompoundDrawablesRelativeWithIntrinsicBounds(0, 0, chevron, 0);
      TextViewCompat.setCompoundDrawableTintList(mSubtitle,
                                                 ColorStateList.valueOf(ThemeUtils.getColor(ctx, R.attr.secondary)));

      if (hasIntermediates && expanded)
      {
        mIntermediates.setVisibility(View.VISIBLE);
        mIntermediates.setText(String.join(" · ", intermediates));
      }
      else
      {
        mIntermediates.setVisibility(View.GONE);
      }
    }
  }
}
