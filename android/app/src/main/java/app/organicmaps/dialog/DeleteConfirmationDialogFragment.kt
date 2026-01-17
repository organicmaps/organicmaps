package app.organicmaps.dialog

import android.app.Dialog
import android.os.Bundle
import androidx.core.os.bundleOf
import androidx.fragment.app.FragmentManager
import app.organicmaps.R
import app.organicmaps.base.BaseMwmDialogFragment
import com.google.android.material.dialog.MaterialAlertDialogBuilder

class DeleteConfirmationDialogFragment : BaseMwmDialogFragment() {

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog =
        MaterialAlertDialogBuilder(requireActivity(), R.style.MwmTheme_AlertDialog)
            .setTitle(getString(R.string.delete_track_dialog_title, requireArguments().getString(EXTRA_NAME)))
            .setPositiveButton(R.string.delete) { _, _ ->
                parentFragmentManager.setFragmentResult(REQUEST_KEY, Bundle.EMPTY)
            }
            .setNegativeButton(R.string.cancel, null)
            .create()

    companion object {
        const val REQUEST_KEY = "DeleteConfirmationResult"

        private const val EXTRA_NAME = "name"
        private const val TAG = "DeleteConfirmationDialogFragment"

        @JvmStatic
        fun showDialog(manager: FragmentManager, name: String) {
            DeleteConfirmationDialogFragment().apply {
                arguments = bundleOf(EXTRA_NAME to name)
                show(manager, TAG)
            }
        }
    }
}
