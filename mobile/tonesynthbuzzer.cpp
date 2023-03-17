/*
    Copyright 2021 Benjamin Vedder	benjamin@vedder.se

    This file is part of VESC Tool.

    VESC Tool is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    VESC Tool is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "tonesynthbuzzer.h"
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
#include <QAudioDeviceInfo>
#include <QAudioOutput>
#else
#include <QMediaDevices>
#include <QAudioDevice>
#include <QAudioSink>
#endif

ToneSynthBuzzer::ToneSynthBuzzer(QObject *parent) : QObject(parent)
{
    m_error = "";
    m_defaultDevice = "";
    m_bufferTime = 256;
    m_sampleRate = 48000;
    m_running = false;

#if !defined(Q_OS_WASM)
    connect(&m_stallDetector, &QTimer::timeout, this, [=]{
        if (m_running) {
            if (m_synth->lastBufferSize() == 0) {
                m_error = "Audio output is stalled right now. Sound cannot be produced. Please increase the buffer time to avoid this problem.";
                m_running = false;
                m_stallDetector.stop();
            }
            m_synth->resetLastBufferSize();
        }
    });
#endif

    SetFormat();
    ScanForDevices();
    StartDevice();
}

ToneSynthBuzzer::~ToneSynthBuzzer()
{
    StopDevice();
}

void ToneSynthBuzzer::SetFormat()
{
    m_format.setSampleRate(m_sampleRate);
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    m_format.setChannelCount(1);
    m_format.setSampleSize(16);
    m_format.setCodec("audio/pcm");
    m_format.setByteOrder(QAudioFormat::LittleEndian);
    m_format.setSampleType(QAudioFormat::SignedInt);
#else
    m_format.setChannelConfig(QAudioFormat::ChannelConfigMono);
    m_format.setSampleFormat(QAudioFormat::Float);
#endif

}

void ToneSynthBuzzer::ScanForDevices()
{
    m_devices.clear();
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    const QAudioDeviceInfo &defaultDeviceInfo = QAudioDeviceInfo::defaultOutputDevice();
    m_devices.insert(defaultDeviceInfo.deviceName(), QVariant::fromValue(defaultDeviceInfo));
    for (auto &deviceInfo: QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
        if (deviceInfo != defaultDeviceInfo && deviceInfo.isFormatSupported(m_format))
            m_devices.insert(deviceInfo.deviceName(), QVariant::fromValue(deviceInfo));
    }
    if (m_defaultDevice == "" || !m_devices.contains(m_defaultDevice))
        m_defaultDevice = defaultDeviceInfo.deviceName();
#else
    const QAudioDevice &defaultDeviceInfo = QMediaDevices::defaultAudioOutput();
    m_devices.insert(defaultDeviceInfo.description(), QVariant::fromValue(defaultDeviceInfo));
    for (auto &deviceInfo: QMediaDevices::audioOutputs()) {
        if (deviceInfo != defaultDeviceInfo && deviceInfo.isFormatSupported(m_format))
            m_devices.insert(deviceInfo.description(), QVariant::fromValue(deviceInfo));
    }
    if (m_defaultDevice == "" || !m_devices.contains(m_defaultDevice))
        m_defaultDevice = defaultDeviceInfo.description();
#endif
}

void ToneSynthBuzzer::InitializeAudio()
{
    ClearError();

    m_synth.reset(new ToneSynthesizer(m_format));

    m_running = false;
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    const QAudioDeviceInfo deviceInfo = m_devices.take(m_defaultDevice).value<QAudioDeviceInfo>();
#else
    const QAudioDevice deviceInfo = m_devices.take(m_defaultDevice).value<QAudioDevice>();
#endif

    if (!deviceInfo.isFormatSupported(m_format) && deviceInfo.deviceName() != "default")
    {
        m_error = "Audio format not supported, The selected audio device does not support the synth's audio format. Please select another device.";
        return;
    }

    qint64 bufferLength = m_format.bytesForDuration( m_bufferTime * 1000 );
    m_synth->start();
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    m_audioOutput.reset(new QAudioOutput(deviceInfo, m_format));
    QObject::connect(m_audioOutput.data(), &QAudioOutput::stateChanged, this, [=](QAudio::State state){
#else
    m_audioOutput.reset(new QAudioSink(deviceInfo, m_format));
    QObject::connect(m_audioOutput.data(), &QAudioSink::stateChanged, this, [=](QAudio::State state){
#endif
        //qDebug() << "Audio Output state:" << state << "error:" << m_audioOutput->error();
        if (m_running && (m_audioOutput->error() == QAudio::UnderrunError)) {
            m_error = "Underrun Error, Audio buffer underrun errors have been detected. Please increase the buffer time to avoid this problem.  ";
        }
    });
    m_audioOutput->setBufferSize(bufferLength);
    m_audioOutput->start(m_synth.get());
#if !defined(Q_OS_WASM)
    auto bufferTime = m_format.durationForBytes(m_audioOutput->bufferSize()) / 1000;
    QTimer::singleShot(bufferTime * 2, this, [=]{
        m_running = true;
        m_stallDetector.start(bufferTime * 4);
     });
#endif
}

void ToneSynthBuzzer::StartDevice()
{
    StopDevice();
    InitializeAudio();
}

void ToneSynthBuzzer::StopDevice()
{
#if !defined(Q_OS_WASM)
    m_stallDetector.stop();
#endif
    m_running = false;
    if(!m_audioOutput.isNull()) {
        m_audioOutput->stop();
    }
    if(!m_synth.isNull()) {
        m_synth->stop();
    }

    if (m_devices.size() > 0)
    {
        m_defaultDevice = m_devices.firstKey();
    }
}

void ToneSynthBuzzer::SetBufferTime(int bufferTime)
{
    if (m_bufferTime != bufferTime)
    {
        m_bufferTime = bufferTime;
        StartDevice();
    }
}

void ToneSynthBuzzer::SetSampleRate(unsigned int sampleRate)
{
    if (m_sampleRate != sampleRate)
    {
        m_sampleRate = sampleRate;
        SetFormat();
        ScanForDevices();
        StartDevice();
    }
}

bool ToneSynthBuzzer::SetDeviceByName(QString name)
{
    if (!m_devices.contains(name))
        return false;
    m_defaultDevice = name;
    StartDevice();
    return true;
}

int ToneSynthBuzzer::GetNumDevices()
{
    return m_devices.size();
}

QList<QString> ToneSynthBuzzer::GetDevicesNames()
{
    return m_devices.keys();
}

QList<QString> ToneSynthBuzzer::GetNotesAvailable()
{
    return m_synth->getNotesAvailable();
}

void ToneSynthBuzzer::SetWaveType(QString type)
{
    if (type == "Sine")
        m_synth->setWaveType(ToneSynthesizer::WaveType::SineWave);
    else if (type == "Saw")
        m_synth->setWaveType(ToneSynthesizer::WaveType::SawWave);
}

void ToneSynthBuzzer::SetVolume(int volume)
{
    qreal linearVolume = QAudio::convertVolume(volume / 100.0,
                                               QAudio::LogarithmicVolumeScale,
                                               QAudio::LinearVolumeScale);
    m_audioOutput->setVolume(linearVolume);
}

void ToneSynthBuzzer::SetOctave(int octave)
{
    m_synth->setOctave(octave);
}

void ToneSynthBuzzer::NoteOn(QString note)
{
    m_synth->noteOn(note);
}

void ToneSynthBuzzer::NoteOff()
{
    m_synth->noteOff();
}

QString ToneSynthBuzzer::GetError()
{
    return m_error;
}

void ToneSynthBuzzer::ClearError()
{
    m_error = "";
}
