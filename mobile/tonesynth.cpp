/*
    Minimal Synthesizer for Qt applications
    Copyright (C) 2022, Pedro Lopez-Cabanillas <plcl@users.sf.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <limits>
//#include <QDebug>
#include <QtMath>
#include "tonesynth.h"

ToneSynthesizer::ToneSynthesizer(const QAudioFormat &format):
    QIODevice(),
    m_waveType(WaveType::SawWave),
    m_octave(3),
    m_envelState(EnvelopeState::silentState),
    m_envelVolume(0.0)
{
    //qDebug() << Q_FUNC_INFO;
    if (format.isValid()) {
        m_format = format;
        m_attackTime = (quint64) (0.02 * format.sampleRate());
        m_releaseTime = m_attackTime;
        m_envelDelta = 1.0 / m_attackTime;
    }
}

void ToneSynthesizer::start()
{
    //qDebug() << Q_FUNC_INFO;
    open(QIODevice::ReadOnly | QIODevice::Unbuffered);
}

void ToneSynthesizer::stop()
{
    //qDebug() << Q_FUNC_INFO;
    if (isOpen()) {
        close();
    }
}

void ToneSynthesizer::noteOn(const QString &note)
{
    //qDebug() << Q_FUNC_INFO << note;
    if (m_freq.contains(note)) {
        qreal noteFreq = qPow(2, m_octave - 3) * m_freq[note];
        qreal cyclesPerSample = noteFreq / m_format.sampleRate();
        m_angleDelta = cyclesPerSample * M_PI; // phase increment
        if (m_waveType == WaveType::SineWave)
        {
            m_angleDelta *= 2.0;
        }
        m_currentAngle = 0.0; // phase

        m_envelState = EnvelopeState::attackState;
        m_envelCount = m_attackTime;
        m_envelVolume = 0.0;
    }
}

void ToneSynthesizer::noteOff()
{
//    qDebug() << Q_FUNC_INFO
//             << "last synth period:"
//             << m_lastBufferSize << "bytes,"
//             << m_format.durationForBytes(m_lastBufferSize) / 1000
//             << "milliseconds";
    m_envelState = EnvelopeState::releaseState;
    m_envelCount = m_releaseTime;
}

qint64 ToneSynthesizer::lastBufferSize() const
{
    return m_lastBufferSize;
}

void ToneSynthesizer::resetLastBufferSize()
{
    m_lastBufferSize = 0;
}

QList<QString> ToneSynthesizer::getNotesAvailable()
{
    return m_freq.keys();
}

void ToneSynthesizer::setWaveType(WaveType type)
{
    m_waveType = type;
}

void ToneSynthesizer::setOctave(int newOctave)
{
    m_octave = newOctave;
}

qint64 ToneSynthesizer::readData(char *data, qint64 maxlen)
{
    //qDebug() << Q_FUNC_INFO << maxlen;
    const qint64 channelBytes =
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
            m_format.sampleSize() / CHAR_BIT;
#else
            m_format.bytesPerSample();
#endif
    Q_ASSERT(channelBytes > 0);
    unsigned char *ptr = reinterpret_cast<unsigned char *>(data);

    qint64 length = maxlen;
    while (length > 0) {
        float currentSample = 0.0;
        switch (m_envelState)
        {
        case EnvelopeState::silentState :
            break;
        case EnvelopeState::attackState :
            if (m_envelCount > 0)
            {
                m_envelVolume += m_envelDelta;
                m_envelCount--;
            }
            else
            {
                m_envelVolume = 1.0;
                m_envelState = EnvelopeState::sustainState;
            }
            break;
        case EnvelopeState::sustainState :
            break;
        case EnvelopeState::releaseState :
            if (m_envelCount > 0)
            {
                m_envelVolume -= m_envelDelta;
                m_envelCount--;
            }
            else
            {
                m_envelVolume = 0.0;
                m_envelState = EnvelopeState::silentState;
            }
            break;
        }

        if (m_envelState != EnvelopeState::silentState) {
            if (m_waveType == WaveType::SineWave)
            {
                currentSample = qSin(m_currentAngle);
            }
            else if (m_waveType == WaveType::SawWave)
            {
                currentSample = qTan(m_currentAngle);
            }
            else
            {
                currentSample = qSin(m_currentAngle);
            }
            currentSample = currentSample * size() * m_envelVolume;
            m_currentAngle += m_angleDelta;
        }
        *reinterpret_cast<qint16 *>(ptr) = currentSample;
        ptr += channelBytes;
        length -= channelBytes;
    }
    m_lastBufferSize = maxlen;
    return maxlen;
}

qint64 ToneSynthesizer::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);
    //qDebug() << Q_FUNC_INFO;
	return 0;
}

qint64 ToneSynthesizer::size() const
{
    //qDebug() << Q_FUNC_INFO;
    return std::numeric_limits<qint16>::max();
}

qint64 ToneSynthesizer::bytesAvailable() const
{
    //qDebug() << Q_FUNC_INFO;
    return std::numeric_limits<qint16>::max();
}
