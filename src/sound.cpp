#include "game.h"
#include <windows.h>
#include <mmsystem.h>
#include <string>
#include <iostream> 

#pragma comment(lib, "winmm.lib") 

void updateMusicVolume()
{
    int mciVol = musicVolume * 10;
    std::string cmd = "setaudio music volume to " + std::to_string(mciVol);
    mciSendString(cmd.c_str(), NULL, 0, NULL);
}

void playBackgroundMusic()
{
    mciSendString(TEXT("open \"assets/audio/Dispatch - Gristle.wav\" type mpegvideo alias music"), NULL, 0, NULL);
    mciSendString(TEXT("play music repeat"), NULL, 0, NULL);
    updateMusicVolume();
}

void playSFX(const char* file)
{
    if (!sfxEnabled) return;
    PlaySound(file, NULL, SND_FILENAME | SND_ASYNC | SND_NOSTOP);
}