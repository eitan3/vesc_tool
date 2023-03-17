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

#ifndef TONESYNTHBUZZER_H
#define TONESYNTHBUZZER_H

#include "tonesynth.h"
#include <QTimer>
#include <QList>
#include <QVariant>
#include <QAudioFormat>
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
#include <QAudioOutput>
#else
#include <QAudioSink>
#endif

class ToneSynthBuzzer : public QObject
{
    Q_OBJECT
public:
    explicit ToneSynthBuzzer(QObject *parent = nullptr);
    ~ToneSynthBuzzer();

    Q_INVOKABLE void ScanForDevices();
    Q_INVOKABLE void StartDevice();
    Q_INVOKABLE void StopDevice();
    Q_INVOKABLE void SetBufferTime(int bufferTime);
    Q_INVOKABLE bool SetDeviceByName(QString name);
    Q_INVOKABLE void SetSampleRate(unsigned int sampleRate);
    Q_INVOKABLE int GetNumDevices();
    Q_INVOKABLE QList<QString> GetDevicesNames();
    Q_INVOKABLE QList<QString> GetNotesAvailable();
    Q_INVOKABLE void SetWaveType(QString type);
    Q_INVOKABLE void SetVolume(int volume);
    Q_INVOKABLE void SetOctave(int octave);
    Q_INVOKABLE void NoteOn(QString note);
    Q_INVOKABLE void NoteOff();
    Q_INVOKABLE QString GetError();
    Q_INVOKABLE void ClearError();

signals:

private:
    // Set QAudioFormat, this function called once by the constructor.
    void SetFormat();

    // Initialize Audio Output.
    void InitializeAudio();

    int m_bufferTime;
    bool m_running;
    unsigned int m_sampleRate;
    QAudioFormat m_format;
    QScopedPointer<ToneSynthesizer> m_synth;
    QString m_defaultDevice;
    QMap<QString, QVariant> m_devices;
    QString m_error;

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    QScopedPointer<QAudioOutput> m_audioOutput;
#else
    QScopedPointer<QAudioSink> m_audioOutput;
#endif

#if !defined(Q_OS_WASM)
    QTimer m_stallDetector;
#endif
};

#endif // TONESYNTHBUZZER_H
