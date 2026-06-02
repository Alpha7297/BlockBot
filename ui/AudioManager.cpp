#include "AudioManager.h"
#include "AppGraphicsView.h"

#include <QAudioOutput>
#include <QMediaPlayer>
#include <QUrl>

namespace{

QMediaPlayer* musicPlayer=nullptr;
QAudioOutput* musicOutput=nullptr;
QMediaPlayer* effectPlayer=nullptr;
QAudioOutput* effectOutput=nullptr;
QString currentTrack;
QString currentEffect;
bool switchToPuzzleWhenCurrentTrackEnds=false;

void ensurePlayer(){
    if(musicPlayer!=nullptr){
        return;
    }
    musicPlayer=new QMediaPlayer();
    musicOutput=new QAudioOutput(musicPlayer);
    musicOutput->setVolume(0.45);
    musicPlayer->setAudioOutput(musicOutput);
    musicPlayer->setLoops(QMediaPlayer::Infinite);
    QObject::connect(musicPlayer,&QMediaPlayer::mediaStatusChanged,
        [](QMediaPlayer::MediaStatus status){
        if(switchToPuzzleWhenCurrentTrackEnds&&status==QMediaPlayer::EndOfMedia){
            switchToPuzzleWhenCurrentTrackEnds=false;
            audio::playPuzzleMusic();
        }
    });
}

void ensureEffectPlayer(){
    if(effectPlayer!=nullptr){
        return;
    }
    effectPlayer=new QMediaPlayer();
    effectOutput=new QAudioOutput(effectPlayer);
    effectOutput->setVolume(0.55);
    effectPlayer->setAudioOutput(effectOutput);
    effectPlayer->setLoops(1);
}

void playEffect(const QString& assetPath){
    ensureEffectPlayer();
    const QString path=loadAsset(assetPath);
    if(path.isEmpty()){
        return;
    }
    if(currentEffect!=path){
        effectPlayer->setSource(QUrl::fromLocalFile(path));
        currentEffect=path;
    }
    effectPlayer->stop();
    effectPlayer->setPosition(0);
    effectPlayer->play();
}

void playTrack(const QString& assetPath){
    ensurePlayer();
    switchToPuzzleWhenCurrentTrackEnds=false;
    musicPlayer->setLoops(QMediaPlayer::Infinite);
    const QString path=loadAsset(assetPath);
    if(path.isEmpty()){
        return;
    }
    if(currentTrack!=path){
        musicPlayer->setSource(QUrl::fromLocalFile(path));
        currentTrack=path;
    }
    if(musicPlayer->playbackState()!=QMediaPlayer::PlayingState){
        musicPlayer->play();
    }
}

}

namespace audio{

void playMenuMusic(){
    playTrack(QStringLiteral("music/menu.mp3"));
}

void playPuzzleMusic(){
    playTrack(QStringLiteral("music/puzzle.mp3"));
}

void playEnterLevelMusic(){
    ensurePlayer();
    switchToPuzzleWhenCurrentTrackEnds=false;
    const QString path=loadAsset(QStringLiteral("music/enter_level.mp3"));
    if(path.isEmpty()){
        return;
    }
    musicPlayer->setLoops(1);
    if(currentTrack!=path){
        musicPlayer->setSource(QUrl::fromLocalFile(path));
        currentTrack=path;
    }
    musicPlayer->play();
}

void playPuzzleMusicAfterCurrentTrack(){
    switchToPuzzleWhenCurrentTrackEnds=true;
}

void playGentleMusic(){
    playTrack(QStringLiteral("music/gentle.mp3"));
}

void playEscapeButLoseMusic(){
    playTrack(QStringLiteral("music/escape_but_lose.mp3"));
}

void playMessageButLoseMusic(){
    playTrack(QStringLiteral("music/message_but_lose.mp3"));
}

void playFinaleMusic(){
    playTrack(QStringLiteral("music/finale.mp3"));
}

void playTypeSound(){
    playEffect(QStringLiteral("music/type.wav"));
}

void playSelectSound(){
    playEffect(QStringLiteral("music/select.wav"));
}

void stopMusic(){
    if(musicPlayer!=nullptr){
        musicPlayer->stop();
    }
}

}
