package com.mapswithme.maps.tests;

import java.util.concurrent.CountDownLatch;

import android.test.ActivityInstrumentationTestCase2;
import android.widget.EditText;
import android.widget.Spinner;

import com.jayway.android.robotium.solo.Solo;
import com.mapswithme.maps.R;
import com.mapswithme.maps.search.SearchActivity;
import com.squareup.spoon.Spoon;

public class SearchActivityTest extends ActivityInstrumentationTestCase2<SearchActivity>
{

  public SearchActivityTest()
  {
    super(SearchActivity.class);
  }

  public void testView_Spinner()
  {
    final SearchActivity activity = getActivity();
    final Spinner spinner = ((Spinner) activity.findViewById(R.id.search_modes_spinner));
    final EditText searchField = (EditText) activity.findViewById(R.id.search_string);
    final Solo solo = new Solo(getInstrumentation(), getActivity());
    solo.clearEditText(searchField);

    Spoon.screenshot(activity, "initial_state");
    solo.clickOnView(spinner);

    final CountDownLatch cdl = new CountDownLatch(1);
    spinner.postDelayed(new Runnable()
    {
      @Override
      public void run()
      {
        cdl.countDown();
      }
    }, 3 * 1000);
    try
    {
      cdl.await();
    }
    catch (InterruptedException e)
    {
      fail("Didn't wait for screenshot");
    }

    Spoon.screenshot(activity, "spinner_clicked");
  }

  public void testSearch_Search()
  {
    final SearchActivity activity = getActivity();
    final Solo solo = new Solo(getInstrumentation(), getActivity());
    final Spinner spinner = ((Spinner) activity.findViewById(R.id.search_modes_spinner));

    final EditText searchField = (EditText) activity.findViewById(R.id.search_string);
    final String[] searchTerms = { "fo", "food", "ресторан", "Ми", "Минск", "Моск", "San", "London", "Cinema" };

    for (String term : searchTerms)
      for (int i = 0; i < 3; i++)
      {
        final int index = i;
        getInstrumentation().runOnMainSync(new Runnable()
        {
          @Override
          public void run()
          {
            spinner.setSelection(index);
          }
        });
        getInstrumentation().waitForIdleSync();

        solo.clearEditText(searchField);
        solo.enterText(searchField, term);

        final CountDownLatch cdl = new CountDownLatch(1);
        searchField.postDelayed(new Runnable()
        {
          @Override
          public void run()
          {
            cdl.countDown();
          }
        }, 1500);
        try
        {
          cdl.await();
        }
        catch (InterruptedException e)
        {
          fail("Didn't wait for screenshot");
        }

        Spoon.screenshot(activity, "search_" + term.hashCode());
      }
  }

}
