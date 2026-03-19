#pragma once
#include "AEAudio.h"

struct BGMManager {
    AEAudioGroup group;      
    AEAudioGroup gachaGroup;  
    AEAudio normalTrack;
    AEAudio eliteTrack;
    AEAudio bossTrack;
    AEAudio gachaTrack;
    AEAudio creditsTrack;

    void Init();
    void PlayNormal();
    void PlayElite();
    void PlayBoss();
    void StopGameplayBGM();  
    void PlayGacha();
    void StopGacha(float fadeTimeSec);
    void PlayCredits();
    void StopCredits();
    void Exit();
};

extern BGMManager bgm;
