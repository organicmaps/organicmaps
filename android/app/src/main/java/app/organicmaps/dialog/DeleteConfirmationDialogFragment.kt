package app.organicmaps.dialog

import android.app.Dialog
import android.os.Bundle
import androidx.core.os.bundleOf
import androidx.fragment.app.FragmentManager
import app.organicmaps.R
import app.organicmaps.base.BaseMwmDialogFragment
import com.google.android.material.dialog.MaterialAlertDialogBuilder

class DeleteConfirmationDialogFragment : BaseMwmDialogFragment() {

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        // Both are always written by showDialog().
        val args = requireArguments()
        val requestKey = checkNotNull(args.getString(EXTRA_REQUEST_KEY))
        return MaterialAlertDialogBuilder(requireActivity(), R.style.MwmTheme_AlertDialog)
            .setTitle(args.getString(EXTRA_TITLE))
            .setPositiveButton(R.string.delete) { _, _ ->
                parentFragmentManager.setFragmentResult(requestKey, Bundle.EMPTY)
            }
            .setNegativeButton(R.string.cancel, null)
            .create()
    }

    companion object {
        const val REQUEST_KEY = "DeleteConfirmationResult"

        private const val EXTRA_TITLE = "title"
        private const val EXTRA_REQUEST_KEY = "requestKey"
        private const val TAG = "DeleteConfirmationDialogFragment"

        /**
         * @param requestKey lets a screen tell several confirmations apart.
         */
        @JvmStatic
        @JvmOverloads
        fun showDialog(manager: FragmentManager, title: String, requestKey: String = REQUEST_KEY) {
            DeleteConfirmationDialogFragment().apply {
                arguments = bundleOf(EXTRA_TITLE to title, EXTRA_REQUEST_KEY to requestKey)
                show(manager, TAG)
            }
        }
    }
}
