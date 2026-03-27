package app.organicmaps.wear;

import android.content.Context;
import androidx.annotation.NonNull;
import app.organicmaps.sdk.routing.RoutingInfo;
import app.organicmaps.sdk.util.log.Logger;
import com.google.android.gms.tasks.Task;
import com.google.android.gms.wearable.DataClient;
import com.google.android.gms.wearable.DataMap;
import com.google.android.gms.wearable.PutDataMapRequest;
import com.google.android.gms.wearable.PutDataRequest;
import com.google.android.gms.wearable.Wearable;

public class WearSyncService {
    private static final String TAG = WearSyncService.class.getSimpleName();
    private static final String PATH_NAVIGATION = "/navigation/status";

    public static void updateNavigation(@NonNull Context context, @NonNull RoutingInfo info) {
        PutDataMapRequest putDataMapReq = PutDataMapRequest.create(PATH_NAVIGATION);
        DataMap map = putDataMapReq.getDataMap();

        map.putString("distToTurn", info.distToTurn.toString(context));
        map.putString("nextStreet", info.nextStreet);
        map.putInt("carDirection", info.carDirection.ordinal());
        map.putInt("exitNum", info.exitNum);
        map.putLong("timestamp", System.currentTimeMillis());

        PutDataRequest putDataReq = putDataMapReq.asPutDataRequest();
        putDataReq.setUrgent();
        
        Task<DataClient.DataItem> putDataTask = Wearable.getDataClient(context).putDataItem(putDataReq);
        putDataTask.addOnFailureListener(e -> Logger.e(TAG, "Failed to send navigation data to Wear", e));
    }
    
    public static void stopNavigation(@NonNull Context context) {
        PutDataMapRequest putDataMapReq = PutDataMapRequest.create(PATH_NAVIGATION);
        DataMap map = putDataMapReq.getDataMap();
        map.putBoolean("active", false);
        map.putLong("timestamp", System.currentTimeMillis());

        PutDataRequest putDataReq = putDataMapReq.asPutDataRequest();
        putDataReq.setUrgent();
        Wearable.getDataClient(context).putDataItem(putDataReq);
    }
}
