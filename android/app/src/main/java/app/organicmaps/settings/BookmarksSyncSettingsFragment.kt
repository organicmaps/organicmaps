package app.organicmaps.settings

import android.accounts.AccountManager
import android.app.Activity
import android.content.ActivityNotFoundException
import android.os.Build
import android.os.Bundle
import android.text.format.DateUtils
import android.text.format.Formatter
import android.view.View
import android.widget.Toast
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
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.launch

class BookmarksSyncSettingsFragment : BaseXmlSettingsFragment() {
    private val shareLauncher = SharingUtils.RegisterLauncher(this)
    private val authorizationLauncher =
        registerForActivityResult(ActivityResultContracts.StartIntentSenderForResult()) { result ->
            val switching = switchingTo
            switchingTo = null
            // Closing the account picker or the consent screen is not an error.
            if (result.resultCode != Activity.RESULT_OK) return@registerForActivityResult
            val provider = CloudSyncRegistry.provider ?: return@registerForActivityResult
            viewLifecycleOwner.lifecycleScope.launch {
                onAuthorizationResult(provider.handleAuthorizationResult(result.data, switching))
            }
        }
    private val accountPickerLauncher =
        registerForActivityResult(ActivityResultContracts.StartActivityForResult()) { result ->
            val accountName = result.data?.getStringExtra(AccountManager.KEY_ACCOUNT_NAME)
            if (result.resultCode != Activity.RESULT_OK || accountName == null) return@registerForActivityResult
            val provider = CloudSyncRegistry.provider ?: return@registerForActivityResult
            viewLifecycleOwner.lifecycleScope.launch {
                onAuthorizationResult(provider.switchAccount(accountName), switchingTo = accountName)
            }
        }

    private lateinit var enabledPref: TwoStatePreference
    private lateinit var statusPref: BookmarksSyncStatusPreference
    private lateinit var accountPref: Preference
    private lateinit var storagePref: Preference
    private lateinit var mobileDataPref: TwoStatePreference
    private lateinit var deletePref: Preference
    private var waiting: BookmarksSync.Waiting? = null

    // The account picked to switch to while its consent screen is open; it survives the recreation of this screen.
    private var switchingTo: String? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        switchingTo = savedInstanceState?.getString(KEY_SWITCHING_TO)
    }

    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)
        outState.putString(KEY_SWITCHING_TO, switchingTo)
    }

    override fun getXmlResources() = R.xml.prefs_bookmarks_sync

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        enabledPref = getPreference(getString(R.string.pref_bookmarks_sync_enabled))
        statusPref = getPreference(getString(R.string.pref_bookmarks_sync_status))
        accountPref = getPreference(getString(R.string.pref_bookmarks_sync_account))
        storagePref = getPreference(getString(R.string.pref_bookmarks_sync_storage))
        mobileDataPref = getPreference(getString(R.string.pref_bookmarks_sync_mobile_data))
        deletePref = getPreference(getString(R.string.pref_bookmarks_sync_delete))

        enabledPref.setOnPreferenceChangeListener { _, newValue ->
            if (newValue == true) requestEnable() else disable()
            // The state changes only after authorization succeeds.
            false
        }
        statusPref.onSyncNow = { BookmarksSync.syncNow(requireContext()) }
        accountPref.setOnPreferenceClickListener {
            if (BookmarksSync.engine(requireContext())?.status?.value == SyncStatus.NeedsSignIn) {
                authorize()
            } else {
                CloudSyncRegistry.provider?.let { accountPickerLauncher.launch(it.chooseAccountIntent()) }
            }
            true
        }
        storagePref.setOnPreferenceClickListener {
            val intent = CloudSyncRegistry.provider?.manageStorageIntent() ?: return@setOnPreferenceClickListener true
            try {
                startActivity(intent)
            } catch (ignored: ActivityNotFoundException) {
                // No browser and no Google One: nothing to open.
            }
            true
        }
        mobileDataPref.isChecked = BookmarksSync.isMobileDataAllowed(requireContext())
        mobileDataPref.setOnPreferenceChangeListener { _, newValue ->
            BookmarksSync.setMobileDataAllowed(requireContext(), newValue == true)
            true
        }
        deletePref.setOnPreferenceClickListener {
            confirmDeleteCloudData()
            true
        }

        BookmarksSync.engine(requireContext())?.let { engine ->
            viewLifecycleOwner.lifecycleScope.launch {
                viewLifecycleOwner.repeatOnLifecycle(Lifecycle.State.STARTED) {
                    val waitingReasons = BookmarksSync.waiting(requireContext())
                    combine(engine.status, waitingReasons, ::Pair).collect { (status, reason) ->
                        waiting = reason
                        showStatus(status)
                    }
                }
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
            .setPositiveButton(R.string.enable) { _, _ ->
                prefs.edit { putBoolean(PREF_BETA_WARNING_SHOWN, true) }
                authorize()
            }
            .setNeutralButton(R.string.export_file) { _, _ ->
                BookmarksSharingHelper.INSTANCE.prepareBookmarkCategoriesForSharing(requireActivity(), shareLauncher)
            }
            .setNegativeButton(R.string.cancel, null)
            .show()
    }

    private fun authorize() {
        val provider = CloudSyncRegistry.provider ?: return
        viewLifecycleOwner.lifecycleScope.launch { onAuthorizationResult(provider.authorize()) }
    }

    private fun onAuthorizationResult(result: AuthResult, switchingTo: String? = null) {
        when (result) {
            AuthResult.Authorized -> {
                BookmarksSync.engine(requireContext())?.onAuthorized()
                if (BookmarksSync.isEnabled(requireContext())) {
                    BookmarksSync.syncNow(requireContext())
                } else {
                    BookmarksSync.enable(requireContext())
                }
                updateEnabledState()
            }

            is AuthResult.NeedsResolution -> {
                this.switchingTo = switchingTo
                authorizationLauncher.launch(IntentSenderRequest.Builder(result.intent).build())
            }

            // On the first enable there is no status row yet.
            is AuthResult.Failed -> Toast.makeText(requireContext(), errorText(result.error), Toast.LENGTH_LONG).show()
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
        viewLifecycleOwner.lifecycleScope.launch {
            val error = BookmarksSync.deleteCloudData(requireContext())
            if (error != null) Toast.makeText(requireContext(), errorText(error), Toast.LENGTH_LONG).show()
            updateEnabledState()
        }
        // Sync is off before the deletion starts.
        updateEnabledState()
    }

    private fun updateEnabledState() {
        val isEnabled = BookmarksSync.isEnabled(requireContext())
        enabledPref.isChecked = isEnabled
        val rows = listOf(statusPref, accountPref, storagePref, mobileDataPref, deletePref)
        for (pref in rows) pref.isVisible = isEnabled
        BookmarksSync.engine(requireContext())?.let { showStatus(it.status.value) }
        accountPref.summary = null
        val provider = CloudSyncRegistry.provider
        if (!isEnabled || provider == null) return
        // A known account comes without suspending, so the email shows up right away.
        viewLifecycleOwner.lifecycleScope.launch { accountPref.summary = provider.accountEmail() }
        viewLifecycleOwner.lifecycleScope.launch {
            val quota = provider.storageQuota() ?: return@launch
            val limit = quota.limit ?: return@launch
            storagePref.summary =
                getString(R.string.bookmarks_sync_storage_used, storageSize(quota.usage), storageSize(limit))
        }
    }

    /** Google counts storage in binary units: a "2 TB" plan has 2 TiB. */
    private fun storageSize(bytes: Long): String {
        // Formatter uses decimal units since Android 8, so the value is scaled to keep Google's numbers.
        val value = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) toDecimalUnits(bytes) else bytes
        return Formatter.formatShortFileSize(requireContext(), value)
    }

    private fun showStatus(status: SyncStatus) {
        val statusText = when (status) {
            is SyncStatus.Idle -> lastSyncText(status.lastSync)
            is SyncStatus.Running -> lastSyncText(status.lastSync) ?: getString(R.string.bookmarks_sync_status_running)
            SyncStatus.NeedsSignIn -> getString(R.string.bookmarks_sync_status_sign_in)
            is SyncStatus.Error -> errorText(status.error)
        }
        val waitingText = when (waiting.takeIf { status !is SyncStatus.Running }) {
            BookmarksSync.Waiting.FOR_CONNECTION -> getString(R.string.common_check_internet_connection_dialog)
            BookmarksSync.Waiting.FOR_WIFI -> getString(R.string.bookmarks_sync_status_waiting_wifi)
            null -> null
        }
        // Before the first sync there is no time to show, so the reason it waits takes its place.
        statusPref.title = statusText ?: waitingText
        statusPref.summary = waitingText.takeIf { statusText != null }
        statusPref.isSyncing = status is SyncStatus.Running
    }

    private fun lastSyncText(lastSync: Long?): String? = lastSync?.let {
        val flags = DateUtils.FORMAT_SHOW_DATE or DateUtils.FORMAT_SHOW_TIME
        val time = DateUtils.formatDateTime(requireContext(), it, flags)
        getString(R.string.bookmarks_sync_status_synced, time)
    }

    private fun errorText(error: SyncError): String = when (error) {
        SyncError.AuthRequired -> getString(R.string.bookmarks_sync_status_sign_in)
        SyncError.QuotaExceeded -> getString(R.string.bookmarks_sync_status_quota)
        SyncError.Transient -> getString(R.string.common_check_internet_connection_dialog)
        is SyncError.Other -> getString(R.string.bookmarks_sync_status_error, error.message)
    }

    private companion object {
        const val PREF_BETA_WARNING_SHOWN = "BookmarksSyncBetaWarningShown"
        const val KEY_SWITCHING_TO = "switching_to"

        fun toDecimalUnits(bytes: Long): Long {
            var value = bytes.toDouble()
            var unit = 1L
            while (value >= BINARY_STEP && unit < MAX_UNIT) {
                value /= BINARY_STEP
                unit *= DECIMAL_STEP
            }
            return (value * unit).toLong()
        }

        const val BINARY_STEP = 1024
        const val DECIMAL_STEP = 1000L
        const val MAX_UNIT = 1_000_000_000_000_000L
    }
}
