package app.organicmaps.editor;

import android.app.Dialog;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.ProgressBar;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.bottomsheet.BottomSheetDialog;
import com.google.android.material.bottomsheet.BottomSheetDialogFragment;
import com.google.android.material.textfield.TextInputEditText;

import app.organicmaps.R;
import app.organicmaps.util.Constants;
import app.organicmaps.util.InputUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.util.concurrency.ThreadPool;
import app.organicmaps.util.concurrency.UiThread;

public class OsmLoginBottomFragment extends BottomSheetDialogFragment {
    final private OsmLoginFragment parentFragment;
    private TextInputEditText mLoginInput;
    private TextInputEditText mPasswordInput;
    private Button mLoginButton;
    private Button mLostPasswordButton;
    private Button mRegisterButton;
    private ProgressBar mProgress;

    public OsmLoginBottomFragment(OsmLoginFragment parentFragment) {
        super();
        this.parentFragment = parentFragment;
    }

    @Override
    public void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_osm_login_bottom, container, false);
    }

    @NonNull
    @Override
    public Dialog onCreateDialog(Bundle savedInstanceState) {
        BottomSheetDialog dialog = (BottomSheetDialog) super.onCreateDialog(savedInstanceState);
        dialog.getBehavior().setState(BottomSheetBehavior.STATE_EXPANDED);
        return dialog;
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        mLoginInput = view.findViewById(R.id.osm_username);
        mPasswordInput = view.findViewById(R.id.osm_password);

        mLoginButton = view.findViewById(R.id.login);
        mLoginButton.setOnClickListener((v) -> doLogin());

        mLostPasswordButton = view.findViewById(R.id.lost_password);
        mLostPasswordButton.setOnClickListener((v) -> Utils.openUrl(requireActivity(), Constants.Url.OSM_RECOVER_PASSWORD));
        mRegisterButton = view.findViewById(R.id.register);
        mRegisterButton.setOnClickListener((v) -> Utils.openUrl(requireActivity(), Constants.Url.OSM_REGISTER));
        mProgress = view.findViewById(R.id.osm_login_progress);
    }

    private void doLogin() {
        InputUtils.hideKeyboard(mLoginInput);
        final String username = mLoginInput.getText().toString().trim();
        final String password = mPasswordInput.getText().toString();
        enableInput(false);
        UiUtils.show(mProgress);
        mLoginButton.setText("");

        ThreadPool.getWorker().execute(() -> {
          final String oauthToken = OsmOAuth.nativeAuthWithPassword(username, password);
          final String username1 = (oauthToken == null) ? null : OsmOAuth.nativeGetOsmUsername(oauthToken);
          UiThread.run(() -> processAuth(oauthToken, username1));
        });
    }

    private void processAuth(String oauthToken, String username)
    {
        if (!isAdded())
            return;

        enableInput(true);
        UiUtils.hide(mProgress);
        mLoginButton.setText(R.string.login_osm);

        if (oauthToken == null)
            parentFragment.onAuthFail();
        else {
            this.dismiss();
            parentFragment.onAuthSuccess(oauthToken, username);
        }
    }

    private void enableInput(boolean enable)
    {
        mPasswordInput.setEnabled(enable);
        mLoginInput.setEnabled(enable);
        mLoginButton.setEnabled(enable);
        mLostPasswordButton.setEnabled(enable);
        mRegisterButton.setEnabled(enable);
    }
}
