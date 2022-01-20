package com.mapswithme.maps.search;

import androidx.annotation.NonNull;

interface HiddenCommand
{
  /**
   * Executes the specified command.
   *
   * @return true if the command has been executed, otherwise - false.
   */
  boolean execute(@NonNull String command);

  abstract class BaseHiddenCommand implements HiddenCommand
  {
    @NonNull
    private final String mCommand;

    BaseHiddenCommand(@NonNull String command)
    {
      mCommand = command;
    }

    @Override
    public boolean execute(@NonNull String command)
    {
      if (!mCommand.equals(command))
        return false;

      executeInternal();
      return true;
    }

    abstract void executeInternal();
  }
}
