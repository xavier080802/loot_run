#include "Music.h"

BGMManager bgm;

void BGMManager::Init() {
	// Background Music groups
	bgmGroup = AEAudioCreateGroup();
	gachaGroup = AEAudioCreateGroup();

	// UI group
	uiGroup = AEAudioCreateGroup();

	// SFX group
	sfxGroup = AEAudioCreateGroup();

	// Load music tracks
	normalTrack = AEAudioLoadMusic("Assets/Audio/Musical, Loop, Epic, Ambient, Woodwind, Voice, Dune SND115714.wav");
	bossTrack = AEAudioLoadMusic("");
	gachaTrack = AEAudioLoadMusic("Assets/Audio/MagicCartoon CTE01_93.3.wav");
	creditsTrack = AEAudioLoadMusic("Assets/Audio/PROSPECTUS - Corporate MSCCRP1_50.wav");

	// Load UI sounds
	uiClickSound = AEAudioLoadMusic("");

	// Load SFX
	attackSound = AEAudioLoadMusic("");
}

// --- Music methods ---
void BGMManager::PlayNormal() {
	AEAudioStopGroup(bgmGroup);
	AEAudioPlay(normalTrack, bgmGroup, 1.0f, 1.0f, -1);
}

void BGMManager::PlayBoss() {
	AEAudioStopGroup(bgmGroup);
	AEAudioPlay(bossTrack, bgmGroup, 1.0f, 1.0f, -1);
}

void BGMManager::StopGameplayBGM() {
	AEAudioStopGroup(bgmGroup);
}

void BGMManager::PlayGacha() {
	AEAudioStopGroup(gachaGroup);
	AEAudioPlay(gachaTrack, gachaGroup, 1.0f, 1.0f, -1);
}

void BGMManager::StopGacha(float) {
	AEAudioStopGroup(gachaGroup);
}

void BGMManager::PlayCredits() {
	AEAudioStopGroup(bgmGroup);
	AEAudioPlay(creditsTrack, bgmGroup, 1.0f, 1.0f, -1);
}

void BGMManager::StopCredits() {
	AEAudioStopGroup(bgmGroup);
}

// --- UI methods ---
void BGMManager::PlayUIClick() {
	AEAudioPlay(uiClickSound, uiGroup, 1.0f, 1.0f, 0);
}

// --- SFX methods ---
void BGMManager::PlayAttack() {
	AEAudioPlay(attackSound, sfxGroup, 1.0f, 1.0f, 0);
}

void BGMManager::PlayExplosion() {
	// AEAudioPlay(explosionSound, sfxGroup, 1.0f, 1.0f, 0);
}

// --- Volume control ---
// Music controls both bgmGroup and gachaGroup so all music tracks scale together
void BGMManager::SetBGMVolume(float v) {
	AEAudioSetGroupVolume(bgmGroup, v);
	AEAudioSetGroupVolume(gachaGroup, v);
}

void BGMManager::SetUIVolume(float v) {
	AEAudioSetGroupVolume(uiGroup, v);
}

void BGMManager::SetSFXVolume(float v) {
	AEAudioSetGroupVolume(sfxGroup, v);
}

void BGMManager::Exit() {
	// Unload music
	AEAudioUnloadAudio(normalTrack);
	AEAudioUnloadAudio(bossTrack);
	AEAudioUnloadAudio(gachaTrack);
	AEAudioUnloadAudio(creditsTrack);

	// Unload UI sounds
	AEAudioUnloadAudio(uiClickSound);

	// Unload SFX
	AEAudioUnloadAudio(attackSound);

	// Unload groups
	AEAudioUnloadAudioGroup(bgmGroup);
	AEAudioUnloadAudioGroup(gachaGroup);
	AEAudioUnloadAudioGroup(uiGroup);
	AEAudioUnloadAudioGroup(sfxGroup);
}
