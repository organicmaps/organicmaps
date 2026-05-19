package app.organicmaps.widget.placepage.sections

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import androidx.lifecycle.ViewModelProvider
import app.organicmaps.R
import app.organicmaps.widget.placepage.PlacePageViewModel

class PlacePageNotesFragment : Fragment() {
    private lateinit var viewModel: PlacePageViewModel

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View {
        viewModel = ViewModelProvider(requireActivity())[PlacePageViewModel::class.java]
        return inflater.inflate(R.layout.place_page_notes_fragment, container, false)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        val widget = view as ExpandableNotesView
        viewModel.mapObject.observe(viewLifecycleOwner) { mapObject ->
            if (mapObject != null) widget.setNotes(mapObject.description)
        }
    }
}
