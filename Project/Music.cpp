#include "Music.h"

BGMManager bgm;

void BGMManager::Init() {
    group = AEAudioCreateGroup();
    normalTrack = AEAudioLoadMusic(
        "Assets/Audio/Musical, Loop, Epic, Ambient, Woodwind, Voice, Dune SND115714.wav"
    );

    gachaGroup = AEAudioCreateGroup();
    gachaTrack = AEAudioLoadMusic(
        "Assets/Audio/MagicCartoon CTE01_93.3.wav"
    );
}

void BGMManager::PlayNormal() {
    AEAudioStopGroup(group);
    AEAudioPlay(normalTrack, group, 1.0f, 1.0f, -1);
}

void BGMManager::PlayElite() {
    AEAudioStopGroup(group);
    AEAudioPlay(eliteTrack, group, 1.0f, 1.0f, -1);
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

void BGMManager::Exit() {
    AEAudioUnloadAudio(normalTrack);
    AEAudioUnloadAudio(eliteTrack);
    AEAudioUnloadAudio(bossTrack);
    AEAudioUnloadAudio(gachaTrack);

    AEAudioUnloadAudioGroup(group);
    AEAudioUnloadAudioGroup(gachaGroup);
}
