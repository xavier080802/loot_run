#pragma once
#include "AEAudio.h"   // include Alpha Engine audio headers

struct BGMManager {
    AEAudioGroup group;
    AEAudio normalTrack;
    AEAudio eliteTrack;
    AEAudio bossTrack;

    void Init();
    void PlayNormal();
    void PlayElite();
    void PlayBoss();
    void FadeTo(AEAudio track, float fadeTimeSec);
    void Exit();
}; 
