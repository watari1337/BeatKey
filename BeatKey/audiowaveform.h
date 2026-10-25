#ifndef AUDIOWAVEFORM_H
#define AUDIOWAVEFORM_H

#include <QObject>
#include <QVector>
#include <QString>
#include <QAudioDecoder>
#include <QAudioBuffer>

/**
 * @brief AudioWaveform decodes audio files (.wav, .mp3, .ogg) and produces
 * normalized amplitude peak envelopes (100 buckets/sec) for timeline rendering.
 */
class AudioWaveform : public QObject
{
    Q_OBJECT

public:
    explicit AudioWaveform(QObject *parent = nullptr);
    ~AudioWaveform() override;

    void load(const QString &audioFilePath);
    void clear();

    bool isReady() const { return m_isReady; }
    int durationMs() const { return m_durationMs; }
    const QVector<float>& peaks() const { return m_peaks; }
    float peakAt(int timeMs) const;

signals:
    void waveformReady();

private slots:
    void handleBufferReady();
    void handleFinished();
    void handleError(QAudioDecoder::Error error);

private:
    bool tryLoadWavPcm(const QString &filePath);
    void generateFallbackWaveform();
    void normalizePeaks();

    QAudioDecoder *m_decoder;
    QString m_filePath;
    QVector<float> m_peaks;
    int m_durationMs;
    bool m_isReady;
};

#endif // AUDIOWAVEFORM_H
