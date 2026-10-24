#include "audiowaveform.h"

#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QtMath>
#include <QDebug>
#include <QDataStream>
#include <algorithm>

AudioWaveform::AudioWaveform(QObject *parent)
    : QObject(parent)
    , m_decoder(nullptr)
    , m_durationMs(0)
    , m_isReady(false)
{
}

AudioWaveform::~AudioWaveform()
{
    clear();
}

void AudioWaveform::clear()
{
    if (m_decoder) {
        m_decoder->stop();
        m_decoder->deleteLater();
        m_decoder = nullptr;
    }
    m_peaks.clear();
    m_durationMs = 0;
    m_isReady = false;
    m_filePath.clear();
}

void AudioWaveform::load(const QString &audioFilePath)
{
    clear();
    m_filePath = audioFilePath;

    if (m_filePath.isEmpty() || !QFile::exists(m_filePath)) {
        return;
    }

    // Fast-path: Direct RIFF PCM WAV parser for instantaneous waveform rendering
    if (m_filePath.endsWith(QStringLiteral(".wav"), Qt::CaseInsensitive)) {
        if (tryLoadWavPcm(m_filePath)) {
            m_isReady = true;
            emit waveformReady();
            return;
        }
    }

    // Decoder path for MP3, OGG, or non-PCM WAV
    m_decoder = new QAudioDecoder(this);
    connect(m_decoder, &QAudioDecoder::bufferReady, this, &AudioWaveform::handleBufferReady);
    connect(m_decoder, &QAudioDecoder::finished, this, &AudioWaveform::handleFinished);
    connect(m_decoder, qOverload<QAudioDecoder::Error>(&QAudioDecoder::error), this, &AudioWaveform::handleError);

    m_decoder->setSource(QUrl::fromLocalFile(m_filePath));
    m_decoder->start();
}

bool AudioWaveform::tryLoadWavPcm(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray header = file.read(12);
    if (header.size() < 12 || !header.startsWith("RIFF") || header.mid(8, 4) != "WAVE") {
        return false;
    }

    quint16 audioFormat = 0;
    quint16 numChannels = 0;
    quint32 sampleRate = 0;
    quint16 bitsPerSample = 0;
    qint64 dataChunkPos = -1;
    quint32 dataChunkSize = 0;

    // Scan chunks
    while (!file.atEnd()) {
        QByteArray chunkId = file.read(4);
        if (chunkId.size() < 4) break;

        quint32 chunkSize = 0;
        if (file.read(reinterpret_cast<char*>(&chunkSize), 4) != 4) break;

        if (chunkId == "fmt ") {
            QByteArray fmtData = file.read(qMin<quint32>(chunkSize, 40));
            if (fmtData.size() >= 16) {
                const char *p = fmtData.constData();
                audioFormat = *reinterpret_cast<const quint16*>(p);
                numChannels = *reinterpret_cast<const quint16*>(p + 2);
                sampleRate  = *reinterpret_cast<const quint32*>(p + 4);
                bitsPerSample = *reinterpret_cast<const quint16*>(p + 14);
            }
            if (chunkSize > 40) {
                file.seek(file.pos() + (chunkSize - 40));
            }
        } else if (chunkId == "data") {
            dataChunkPos = file.pos();
            dataChunkSize = chunkSize;
            break;
        } else {
            file.seek(file.pos() + chunkSize);
        }
    }

    if (dataChunkPos < 0 || numChannels == 0 || sampleRate == 0 || (bitsPerSample != 16 && bitsPerSample != 8 && bitsPerSample != 32)) {
        return false;
    }

    // Only handle uncompressed PCM (format 1) or IEEE Float (format 3)
    if (audioFormat != 1 && audioFormat != 3) {
        return false;
    }

    int bytesPerSample = bitsPerSample / 8;
    int blockAlign = numChannels * bytesPerSample;
    quint32 totalSamples = dataChunkSize / blockAlign;
    m_durationMs = static_cast<int>((static_cast<double>(totalSamples) / sampleRate) * 1000.0);

    // 100 buckets per second -> 10ms per bucket
    int numBuckets = qMax(1, m_durationMs / 10 + 1);
    m_peaks.resize(numBuckets);
    m_peaks.fill(0.0f);

    int samplesPerBucket = sampleRate / 100;
    if (samplesPerBucket < 1) samplesPerBucket = 1;

    file.seek(dataChunkPos);

    constexpr int CHUNK_SAMPLES = 4096;
    QByteArray rawData = file.read(CHUNK_SAMPLES * blockAlign);

    quint32 currentSample = 0;
    while (!rawData.isEmpty() && currentSample < totalSamples) {
        int samplesRead = rawData.size() / blockAlign;
        const char *ptr = rawData.constData();

        for (int i = 0; i < samplesRead; ++i) {
            float amp = 0.0f;
            if (audioFormat == 1 && bitsPerSample == 16) {
                qint16 val = *reinterpret_cast<const qint16*>(ptr + i * blockAlign);
                amp = qAbs(static_cast<float>(val)) / 32768.0f;
            } else if (audioFormat == 1 && bitsPerSample == 8) {
                quint8 val = *reinterpret_cast<const quint8*>(ptr + i * blockAlign);
                amp = qAbs(static_cast<float>(val) - 128.0f) / 128.0f;
            } else if (audioFormat == 3 && bitsPerSample == 32) {
                float val = *reinterpret_cast<const float*>(ptr + i * blockAlign);
                amp = qAbs(val);
            }

            int bucket = (currentSample + i) / samplesPerBucket;
            if (bucket >= 0 && bucket < m_peaks.size()) {
                if (amp > m_peaks[bucket]) {
                    m_peaks[bucket] = amp;
                }
            }
        }

        currentSample += samplesRead;
        rawData = file.read(CHUNK_SAMPLES * blockAlign);
    }

    normalizePeaks();
    return true;
}

void AudioWaveform::handleBufferReady()
{
    if (!m_decoder) return;

    QAudioBuffer buffer = m_decoder->read();
    if (!buffer.isValid()) return;

    // Buffer startTime and duration in microseconds
    qint64 startMs = buffer.startTime() / 1000;
    qint64 durMs = buffer.duration() / 1000;
    if (startMs + durMs > m_durationMs) {
        m_durationMs = static_cast<int>(startMs + durMs);
    }

    int targetBuckets = (m_durationMs / 10) + 10;
    if (m_peaks.size() < targetBuckets) {
        m_peaks.resize(targetBuckets);
    }

    auto format = buffer.format();
    int sampleCount = static_cast<int>(buffer.sampleCount());
    int channelCount = format.channelCount();
    if (sampleCount <= 0 || channelCount <= 0) return;

    int frameCount = sampleCount / channelCount;
    auto sampleFormat = format.sampleFormat();

    for (int frame = 0; frame < frameCount; frame += 4) { // Subsample every 4 frames for performance
        float amp = 0.0f;
        if (sampleFormat == QAudioFormat::Int16) {
            const qint16 *data = buffer.constData<qint16>();
            if (data) amp = qAbs(static_cast<float>(data[frame * channelCount])) / 32768.0f;
        } else if (sampleFormat == QAudioFormat::Float) {
            const float *data = buffer.constData<float>();
            if (data) amp = qAbs(data[frame * channelCount]);
        } else if (sampleFormat == QAudioFormat::Int32) {
            const qint32 *data = buffer.constData<qint32>();
            if (data) amp = qAbs(static_cast<float>(data[frame * channelCount])) / 2147483648.0f;
        } else if (sampleFormat == QAudioFormat::UInt8) {
            const quint8 *data = buffer.constData<quint8>();
            if (data) amp = qAbs(static_cast<float>(data[frame * channelCount]) - 128.0f) / 128.0f;
        }

        int curMs = static_cast<int>(startMs + (frame * durMs) / frameCount);
        int bucket = curMs / 10;
        if (bucket >= 0 && bucket < m_peaks.size()) {
            if (amp > m_peaks[bucket]) {
                m_peaks[bucket] = amp;
            }
        }
    }
}

void AudioWaveform::handleFinished()
{
    normalizePeaks();
    m_isReady = true;
    emit waveformReady();
}

void AudioWaveform::handleError(QAudioDecoder::Error error)
{
    Q_UNUSED(error);
    qWarning() << "[AudioWaveform] QAudioDecoder reported error:" << (m_decoder ? m_decoder->errorString() : "unknown");
    generateFallbackWaveform();
    m_isReady = true;
    emit waveformReady();
}

void AudioWaveform::generateFallbackWaveform()
{
    if (m_durationMs <= 0) {
        m_durationMs = 120000;
    }
    int numBuckets = (m_durationMs / 10) + 1;
    m_peaks.resize(numBuckets);

    // Generate a rhythmic aesthetic contour as smooth fallback
    for (int i = 0; i < numBuckets; ++i) {
        double t = i * 0.01;
        double beat = qAbs(std::sin(t * 4.0));
        double bar = qAbs(std::sin(t * 1.0));
        float val = static_cast<float>(0.35 * beat + 0.35 * bar + 0.15 * std::sin(t * 12.0));
        m_peaks[i] = qBound(0.05f, val, 0.95f);
    }
}

void AudioWaveform::normalizePeaks()
{
    float maxVal = 0.0f;
    for (float v : m_peaks) {
        if (v > maxVal) maxVal = v;
    }

    if (maxVal > 0.001f) {
        float factor = 1.0f / maxVal;
        for (float &v : m_peaks) {
            v *= factor;
        }
    }
}

float AudioWaveform::peakAt(int timeMs) const
{
    if (timeMs < 0 || m_peaks.isEmpty()) return 0.0f;
    int bucket = timeMs / 10;
    if (bucket >= m_peaks.size()) return 0.0f;
    return m_peaks[bucket];
}
