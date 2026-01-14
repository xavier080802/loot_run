#include "Music.h"

void BGMManager::Init() {
    group = AEAudioCreateGroup();
    normalTrack = AEAudioLoadMusic("Assets/Audio/Musical, Loop, Epic, Ambient, Woodwind, Voice, Dune SND115714.wav");
    //eliteTrack = AEAudioLoadMusic("Assets/Audio/bgm_elite.mp3");
   // bossTrack = AEAudioLoadMusic("Assets/Audio/bgm_boss.mp3");
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

void BGMManager::FadeTo(AEAudio track, float fadeTimeSec) {
    // simple fade logic (instant for now)
    AEAudioSetGroupVolume(group, 0.0f);
    AEAudioStopGroup(group);
    AEAudioPlay(track, group, 1.0f, 1.0f, -1);
    AEAudioSetGroupVolume(group, 1.0f);
}

void BGMManager::Exit() {
    AEAudioUnloadAudio(normalTrack);
    AEAudioUnloadAudio(eliteTrack);
    AEAudioUnloadAudio(bossTrack);
    AEAudioUnloadAudioGroup(group);
}