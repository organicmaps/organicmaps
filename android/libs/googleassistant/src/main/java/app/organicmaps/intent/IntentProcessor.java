package app.organicmaps.intent;

import app.organicmaps.intent.geo.handlers.AvoidActionHandler;
import app.organicmaps.intent.geo.handlers.ControlActionHandler;
import app.organicmaps.intent.geo.handlers.NavigationActionHandler;
import app.organicmaps.intent.geo.handlers.ReportActionHandler;
import app.organicmaps.intent.geo.handlers.SearchActionHandler;
import app.organicmaps.intent.geo.handlers.VoiceActionHandler;

public interface IntentProcessor extends AvoidActionHandler, ControlActionHandler, NavigationActionHandler,
                                         ReportActionHandler, SearchActionHandler,
                                         VoiceActionHandler
{}
