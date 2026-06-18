#include "VideoStream.hpp"

#include <QMediaPlayer>
#include <QVideoWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QUrl>
#include <QDebug>


VideoStream::VideoStream(const QString& rtspUrl, QWidget* parent)
    : QWidget(parent), m_url(rtspUrl)
{
    //Constructor
    setupUi();
    applyRtspOptions();

    connect(m_player, &QMediaPlayer::playbackStateChanged,
            this,     &VideoStream::onPlaybackStateChanged);

    connect(m_player, &QMediaPlayer::errorOccurred,
            this,     &VideoStream::onErrorOccurred);
}

void VideoStream::play()
{
    //API - open connection
    m_overlay->setText("Connecting…");
    m_overlay->show();

    m_player->setSource(QUrl(m_url));
    m_player->play();
}

void VideoStream::stop()
{
    m_player->stop();
    m_player->setSource(QUrl());
}

bool VideoStream::isPlaying() const
{
    return m_player->playbackState() == QMediaPlayer::PlayingState;
}


void VideoStream::setupUi()
{
    //Configuration of UI
    setWindowTitle("FPV Stream");
    resize(640, 480);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Video surface
    m_video = new QVideoWidget(this);
    m_video->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(m_video);

    // Overlay label (shown during connecting / on error)
    m_overlay = new QLabel("No stream", this);
    m_overlay->setAlignment(Qt::AlignCenter);
    m_overlay->setStyleSheet(
        "QLabel {"
        "  color: white;"
        "  background: rgba(0,0,0,160);"
        "  font-size: 16px;"
        "  padding: 8px 16px;"
        "}"
    );
    // Position centered
    m_overlay->setParent(m_video);
    m_overlay->adjustSize();
    m_overlay->move(
        (m_video->width()  - m_overlay->width())  / 2,
        (m_video->height() - m_overlay->height()) / 2
    );
    m_overlay->show();

    // Set video output
    m_player = new QMediaPlayer(this);
    m_player->setVideoOutput(m_video);
}


void VideoStream::applyRtspOptions()
{
    // If camera input requires any config, it will be set here
    qDebug() << "[VideoStream] RTSP URL:" << m_url;
}


void VideoStream::onPlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    //Used by isPlaying, prints current state to console
    switch (state) {
        case QMediaPlayer::PlayingState:
            qDebug() << "[VideoStream] Playing";
            m_overlay->hide();
            emit streamStarted();
            break;

        case QMediaPlayer::PausedState:
            m_overlay->setText("Paused");
            m_overlay->show();
            break;

        case QMediaPlayer::StoppedState:
            qDebug() << "[VideoStream] Stopped";
            m_overlay->setText("Stopped");
            m_overlay->show();
            break;
    }
}

void VideoStream::onErrorOccurred(QMediaPlayer::Error error, const QString& errorString)
{
    //Error handling
    Q_UNUSED(error)
    qWarning() << "[VideoStream] Error:" << errorString;
    m_overlay->setText("Error: " + errorString);
    m_overlay->show();
    emit streamError(errorString);
}
