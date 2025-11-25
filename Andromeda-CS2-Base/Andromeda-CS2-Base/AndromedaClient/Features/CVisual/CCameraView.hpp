#pragma once

class CCSGOInput;

class CCameraView
{
public:
    static auto SetThirdPerson(CCSGOInput* input, bool enable) -> void;
};