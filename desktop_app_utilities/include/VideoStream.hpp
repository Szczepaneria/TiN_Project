#pragma once

#include <QWidget>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QUrl>

/**
 * VideoStream
 *
 * A self-contained Qt widget that connects to an RTSP stream
 * (AMB82 FpvSystem) and renders it in a window.
 *
 * Usage:
 *   VideoStream w("rtsp://192.168.4.1/live");
 *   w.show();
 *   w.play();
 */
class VideoStream : public QWidget
{
    Q_OBJECT

public:
    explicit VideoStream(const QString& rtspUrl, QWidget* parent = nullptr);
    ~VideoStream() override = default;

    //Start / resume playback
    void play();

    //Stop playback
    void stop();

    // State Flag
    bool isPlaying() const;

signals:
    // Emitted when the stream becomes active (first frame decoded)
    void streamStarted();

    // Emitted on any unrecoverable media error
    void streamError(const QString& message);

private slots:
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);
    void onErrorOccurred(QMediaPlayer::Error error, const QString& errorString);

private:
    void setupUi();
    void applyRtspOptions();

    QString         m_url;
    QMediaPlayer*   m_player   = nullptr;
    QVideoWidget*   m_video    = nullptr;
    QLabel*         m_overlay  = nullptr;  // status / error text
};
