#include "Music.h"

BGMManager bgm;

void BGMManager::Init() {
    // Groups for background music
    group = AEAudioCreateGroup();
    gachaGroup = AEAudioCreateGroup();

    // Load background music
    normalTrack = AEAudioLoadMusic(
        "Assets/Audio/Musical, Loop, Epic, Ambient, Woodwind, Voice, Dune SND115714.wav"
    );
    bossTrack = AEAudioLoadMusic(
        ""
    );
    gachaTrack = AEAudioLoadMusic(
        "Assets/Audio/MagicCartoon CTE01_93.3.wav"
    );
    creditsTrack = AEAudioLoadMusic(
        "Assets/Audio/PROSPECTUS - Corporate MSCCRP1_50.wav"
    );

    // Create a separate group for sound effects
    sfxGroup = AEAudioCreateGroup();

    // Load sound effects
    attackSound = AEAudioLoadMusic("");
    uiClickSound = AEAudioLoadMusic("");
}

void BGMManager::PlayNormal() {
    AEAudioStopGroup(group);
    AEAudioPlay(normalTrack, group, 1.0f, 1.0f, -1);
}

void BGMManager::PlayBoss() {
    AEAudioStopGroup(group);
    AEAudioPlay(bossTrack, group, 1.0f, 1.0f, -1);
}

void BGMManager::StopGameplayBGM() {
    AEAudioStopGroup(group);
}

void BGMManager::PlayGacha() {
    AEAudioStopGroup(gachaGroup);
    AEAudioPlay(gachaTrack, gachaGroup, 1.0f, 1.0f, -1);
}

void BGMManager::StopGacha(float) {
    AEAudioStopGroup(gachaGroup);
}

void BGMManager::PlayCredits() {
    AEAudioStopGroup(group);
    AEAudioPlay(creditsTrack, group, 1.0f, 1.0f, -1);
}

void BGMManager::StopCredits() {
    AEAudioStopGroup(group);
}

// --- Sound effect methods ---
void BGMManager::PlayAttack() {
    AEAudioPlay(attackSound, sfxGroup, 1.0f, 1.0f, 0);
}


void BGMManager::PlayUIClick() {
    AEAudioPlay(uiClickSound, sfxGroup, 1.0f, 1.0f, 0);
}

void BGMManager::Exit() {
    // Unload BGM
    AEAudioUnloadAudio(normalTrack);
    AEAudioUnloadAudio(bossTrack);
    AEAudioUnloadAudio(gachaTrack);
    AEAudioUnloadAudio(creditsTrack);

    // Unload SFX
    AEAudioUnloadAudio(attackSound);
    AEAudioUnloadAudio(uiClickSound);

    // Unload groups
    AEAudioUnloadAudioGroup(group);
    AEAudioUnloadAudioGroup(gachaGroup);
    AEAudioUnloadAudioGroup(sfxGroup);
}