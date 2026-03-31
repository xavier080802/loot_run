#pragma once
#include "AEAudio.h"
#include <map>
#include <string>

struct BGMManager {
    // Audio Groups
    AEAudioGroup bgmGroup;       // Music: normal, boss, credits
    AEAudioGroup gachaGroup;  // Music: gacha track
    AEAudioGroup uiGroup;     // UI: button clicks, hover sounds
    AEAudioGroup sfxGroup;    // SFX: attack, explosion, gameplay sounds

    // Background Music Tracks
    AEAudio normalTrack;
    AEAudio bossTrack;
    AEAudio gachaTrack;
    AEAudio creditsTrack;

    // UI Sound Tracks
    AEAudio uiHoverSound;
    AEAudio uiClickSound;

    // SFX Tracks
    AEAudio attackSound;
    AEAudio explosionSound;

    std::map<std::string, AEAudio> sfxMap;

    void Init();
    void Exit();

    // BGM Methods
    void PlayNormal();
    void PlayBoss();
    void StopGameplayBGM();
    void PlayGacha();
    void StopGacha(float fadeTimeSec);
    void PlayCredits();
    void StopCredits();

    void PlayClip(std::string const& filepath, float vol = 1.f, float pitch = 1.f, bool isUI = false);

    // UI Methods
    void PlayUIHover();
    void PlayUIClick();

    // SFX Methods
    void PlayAttack();
    void PlayExplosion();

    // Volume control (0.0f = silent, 1.0f = full)
    void SetBGMVolume(float v);
    void SetUIVolume(float v);
    void SetSFXVolume(float v);
};

extern BGMManager bgm;
