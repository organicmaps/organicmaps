/**
 * Name        : Sloynik
 * Version     : 
 * Vendor      : 
 * Description : 
 */


#include "Sloynik.h"
#include "SearchForm.h"

using namespace Osp::App;
using namespace Osp::Base;
using namespace Osp::System;
using namespace Osp::Ui;
using namespace Osp::Ui::Controls;

Sloynik::Sloynik()
{
}

Sloynik::~Sloynik()
{
}

Application*
Sloynik::CreateInstance(void)
{
	// Create the instance through the constructor.
	return new Sloynik();
}

bool
Sloynik::OnAppInitializing(AppRegistry& appRegistry)
{
	// TODO:
	// Initialize UI resources and application specific data.
	// The application's permanent data and context can be obtained from the appRegistry.
	//
	// If this method is successful, return true; otherwise, return false.
	// If this method returns false, the application will be terminated.

	// Uncomment the following statement to listen to the screen on/off events.
	//PowerManager::SetScreenEventListener(*this);

	// Create a form
	SearchForm * pSearchForm = new SearchForm();
	pSearchForm->Initialize();

	// Add the form to the frame
	Frame *pFrame = GetAppFrame()->GetFrame();
	pFrame->AddControl(*pSearchForm);

	// Set the current form
	pFrame->SetCurrentForm(*pSearchForm);

	// Draw and Show the form
	pSearchForm->Draw();
	pSearchForm->Show();

	return true;
}

bool
Sloynik::OnAppTerminating(AppRegistry& appRegistry, bool forcedTermination)
{
	// TODO:
	// Deallocate resources allocated by this application for termination.
	// The application's permanent data and context can be saved via appRegistry.
	return true;
}

void
Sloynik::OnForeground(void)
{
	// TODO:
	// Start or resume drawing when the application is moved to the foreground.
}

void
Sloynik::OnBackground(void)
{
	// TODO:
	// Stop drawing when the application is moved to the background.
}

void
Sloynik::OnLowMemory(void)
{
	// TODO:
	// Free unused resources or close the application.
}

void
Sloynik::OnBatteryLevelChanged(BatteryLevel batteryLevel)
{
	// TODO:
	// Handle any changes in battery level here.
	// Stop using multimedia features(camera, mp3 etc.) if the battery level is CRITICAL.
}

void
Sloynik::OnScreenOn (void)
{
	// TODO:
	// Get the released resources or resume the operations that were paused or stopped in OnScreenOff().
}

void
Sloynik::OnScreenOff (void)
{
	// TODO:
	//  Unless there is a strong reason to do otherwise, release resources (such as 3D, media, and sensors) to allow the device to enter the sleep mode to save the battery.
	// Invoking a lengthy asynchronous method within this listener method can be risky, because it is not guaranteed to invoke a callback before the device enters the sleep mode.
	// Similarly, do not perform lengthy operations in this listener method. Any operation must be a quick one.
}
