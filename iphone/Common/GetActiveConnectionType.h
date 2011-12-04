#pragma once

enum TActiveConnectionType
{
  ENotConnected,
  EConnectedByWiFi,
  EConnectedBy3G
};

TActiveConnectionType GetActiveConnectionType();
