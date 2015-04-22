/*

Copyright (c) 2014, Project OSRM contributors
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef TURN_INSTRUCTIONS_HPP
#define TURN_INSTRUCTIONS_HPP

enum class TurnInstruction : unsigned char
{
    NoTurn = 0,
    GoStraight,
    TurnSlightRight,
    TurnRight,
    TurnSharpRight,
    UTurn,
    TurnSharpLeft,
    TurnLeft,
    TurnSlightLeft,
    ReachViaLocation,
    HeadOn,
    EnterRoundAbout,
    LeaveRoundAbout,
    StayOnRoundAbout,
    StartAtEndOfStreet,
    ReachedYourDestination,
    EnterAgainstAllowedDirection,
    LeaveAgainstAllowedDirection,
    InverseAccessRestrictionFlag = 127,
    AccessRestrictionFlag = 128,
    AccessRestrictionPenalty = 129
};

struct TurnInstructionsClass
{
    TurnInstructionsClass() = delete;
    TurnInstructionsClass(const TurnInstructionsClass &) = delete;

    static inline TurnInstruction GetTurnDirectionOfInstruction(const double angle)
    {
        if (angle >= 23 && angle < 67)
        {
            return TurnInstruction::TurnSharpRight;
        }
        if (angle >= 67 && angle < 113)
        {
            return TurnInstruction::TurnRight;
        }
        if (angle >= 113 && angle < 158)
        {
            return TurnInstruction::TurnSlightRight;
        }
        if (angle >= 158 && angle < 202)
        {
            return TurnInstruction::GoStraight;
        }
        if (angle >= 202 && angle < 248)
        {
            return TurnInstruction::TurnSlightLeft;
        }
        if (angle >= 248 && angle < 292)
        {
            return TurnInstruction::TurnLeft;
        }
        if (angle >= 292 && angle < 336)
        {
            return TurnInstruction::TurnSharpLeft;
        }
        return TurnInstruction::UTurn;
    }

    static inline bool TurnIsNecessary(const TurnInstruction turn_instruction)
    {
        if (TurnInstruction::NoTurn == turn_instruction ||
            TurnInstruction::StayOnRoundAbout == turn_instruction)
        {
            return false;
        }
        return true;
    }
};

#endif /* TURN_INSTRUCTIONS_HPP */
