package app.organicmaps.settings

import android.os.Bundle
import android.text.format.DateUtils
import android.view.View
import androidx.activity.result.IntentSenderRequest
import androidx.activity.result.contract.ActivityResultContracts
import androidx.core.content.edit
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.lifecycleScope
import androidx.lifecycle.repeatOnLifecycle
import androidx.preference.Preference
import androidx.preference.TwoStatePreference
import app.organicmaps.MwmApplication
import app.organicmaps.R
import app.organicmaps.bookmarks.BookmarksSharingHelper
import app.organicmaps.sdk.sync.AuthResult
import app.organicmaps.sdk.sync.CloudSyncRegistry
import app.organicmaps.sdk.sync.SyncError
import app.organicmaps.sdk.sync.SyncStatus
import app.organicmaps.sync.BookmarksSync
import app.organicmaps.util.SharingUtils
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import kotlinx.coroutines.launch

class BookmarksSyncSettingsFragment : BaseXmlSettingsFragment() {
    private val shareLauncher = SharingUtils.RegisterLauncher(this)
    private val authorizationLauncher =
        registerForActivityResult(ActivityResultContracts.StartIntentSenderForResult()) { result ->
            CloudSyncRegistry.provider?.let { onAuthorizationResult(it.handleAuthorizationResult(result.data)) }
        }

    private lateinit var enabledPref: TwoStatePreference
    private lateinit var accountPref: Preference
    private lateinit var statusPref: Preference
    private lateinit var syncNowPref: Preference
    private lateinit var deletePref: Preference

    override fun getXmlResources() = R.xml.prefs_bookmarks_sync

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        enabledPref = getPreference(getString(R.string.pref_bookmarks_sync_enabled))
        accountPref = getPreference(getString(R.string.pref_bookmarks_sync_account))
        statusPref = getPreference(getString(R.string.pref_bookmarks_sync_status))
        syncNowPref = getPreference(getString(R.string.pref_bookmarks_sync_now))
        deletePref = getPreference(getString(R.string.pref_bookmarks_sync_delete))

        enabledPref.setOnPreferenceChangeListener { _, newValue ->
            if (newValue == true) requestEnable() else disable()
            // The state changes only after authorization succeeds.
            false
        }
        statusPref.setOnPreferenceClickListener {
            if (BookmarksSync.engine(requireContext())?.status?.value == SyncStatus.NeedsSignIn) authorize()
            true
        }
        syncNowPref.setOnPreferenceClickListener {
            BookmarksSync.syncNow(requireContext())
            true
        }
        deletePref.setOnPreferenceClickListener {
            confirmDeleteCloudData()
            true
        }

        BookmarksSync.engine(requireContext())?.let { engine ->
            viewLifecycleOwner.lifecycleScope.launch {
                viewLifecycleOwner.repeatOnLifecycle(Lifecycle.State.STARTED) { engine.status.collect(::showStatus) }
            }
        }
        updateEnabledState()
    }

    private fun requestEnable() {
        val prefs = MwmApplication.prefs(requireContext())
        if (prefs.getBoolean(PREF_BETA_WARNING_SHOWN, false)) {
            authorize()
            return
        }
        MaterialAlertDialogBuilder(requireContext(), R.style.MwmTheme_AlertDialog)
            .setTitle(R.string.bookmarks_sync_title)
            .setMessage(R.string.bookmarks_sync_beta_warning)
            .setPositiveButton(R.string.bookmarks_sync_enable) { _, _ ->
                prefs.edit { putBoolean(PREF_BETA_WARNING_SHOWN, true) }
                authorize()
            }
            .setNeutralButton(R.string.bookmarks_sync_export) { _, _ ->
                BookmarksSharingHelper.INSTANCE.prepareBookmarkCategoriesForSharing(requireActivity(), shareLauncher)
            }
            .setNegativeButton(R.string.cancel, null)
            .show()
    }

    private fun authorize() {
        val provider = CloudSyncRegistry.provider ?: return
        viewLifecycleOwner.lifecycleScope.launch { onAuthorizationResult(provider.authorize()) }
    }

    private fun onAuthorizationResult(result: AuthResult) {
        when (result) {
            AuthResult.Authorized -> {
                if (BookmarksSync.isEnabled(requireContext())) {
                    BookmarksSync.syncNow(requireContext())
                } else {
                    BookmarksSync.setEnabled(requireContext(), true)
                }
                updateEnabledState()
            }

            is AuthResult.NeedsResolution -> authorizationLauncher.launch(
                IntentSenderRequest.Builder(result.intent).build(),
            )

            is AuthResult.Failed -> showStatus(SyncStatus.Error(result.error))
        }
    }

    private fun disable() {
        BookmarksSync.disable(requireContext())
        updateEnabledState()
    }

    private fun confirmDeleteCloudData() {
        MaterialAlertDialogBuilder(requireContext(), R.style.MwmTheme_AlertDialog)
            .setTitle(R.string.bookmarks_sync_delete_cloud)
            .setMessage(R.string.bookmarks_sync_delete_cloud_confirm)
            .setPositiveButton(R.string.delete) { _, _ -> deleteCloudData() }
            .setNegativeButton(R.string.cancel, null)
            .show()
    }

    private fun deleteCloudData() {
        BookmarksSync.deleteCloudData(requireContext())
        updateEnabledState()
    }

    private fun updateEnabledState() {
        val isEnabled = BookmarksSync.isEnabled(requireContext())
        enabledPref.isChecked = isEnabled
        for (pref in listOf(statusPref, syncNowPref, deletePref)) pref.isVisible = isEnabled
        accountPref.isVisible = false
        val provider = CloudSyncRegistry.provider
        if (!isEnabled || provider == null) return
        viewLifecycleOwner.lifecycleScope.launch {
            val email = provider.accountEmail() ?: return@launch
            accountPref.title = email
            accountPref.isVisible = true
        }
    }

    private fun showStatus(status: SyncStatus) {
        statusPref.title = when (status) {
            is SyncStatus.Idle -> status.lastSync?.let {
                val time = DateUtils.formatDateTime(
                    requireContext(),
                    it,
                    DateUtils.FORMAT_SHOW_DATE or DateUtils.FORMAT_SHOW_TIME,
                )
                getString(R.string.bookmarks_sync_status_synced, time)
            }

            SyncStatus.Running -> getString(R.string.bookmarks_sync_status_running)

            SyncStatus.NeedsSignIn -> getString(R.string.bookmarks_sync_status_sign_in)

            is SyncStatus.Error -> when (val error = status.error) {
                SyncError.AuthRequired -> getString(R.string.bookmarks_sync_status_sign_in)

                SyncError.QuotaExceeded -> getString(R.string.bookmarks_sync_status_quota)

                // WorkManager retries; the last known state stays on screen.
                SyncError.Transient -> return

                is SyncError.Other -> getString(R.string.bookmarks_sync_status_error, error.message)
            }
        }
        statusPref.isVisible = BookmarksSync.isEnabled(requireContext()) && statusPref.title != null
    }

    private companion object {
        const val PREF_BETA_WARNING_SHOWN = "BookmarksSyncBetaWarningShown"
    }
}
