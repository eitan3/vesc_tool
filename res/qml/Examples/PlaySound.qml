import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.2
import Vedder.vesc.utility 1.0
import Qt.labs.settings 1.0

import Vedder.vesc.tonesynthbuzzer 1.0

Item {
    id: mainItem
    anchors.fill: parent
    
    property var enable_buzzer_dc: false
    property var buzzer_dc_threshold: 90
    property var buzzer_error_text: ""
    property var buzzer_volume: 50
    property var audio_notes: []
    property var audio_devs: []
    
    ToneSynthBuzzer {
        id: mBuzzer
    }
    
    Settings {
        id: settingStorage
    }
    
    Component.onCompleted: {
        // Add wave type options
        dcWaveTypeModel.append({"text": "Sine Wave"})
        dcWaveTypeModel.append({"text": "Saw Wave"})

        // Get available notes and load the last selected note
        audio_notes = mBuzzer.GetNotesAvailable();
        for(var i in audio_notes){
            buzzerDcNoteModel.append({ "text": audio_notes[i] })
        }
        buzzerDcNoteCB.currentIndex = settingStorage.value("dc_note", 0)

        // Scan audio devices and load the last used device
        mBuzzer.ScanForDevices()
        audio_devs = mBuzzer.GetDevicesNames()
        for(var i in audio_devs){
            audioDevicesDbModel.append({ "text": audio_devs[i] })
        }
        audioDevicesDownBox.currentIndex = settingStorage.value("audio_device", 0)
        mBuzzer.SetDeviceByName(audio_devs[audioDevicesDownBox.currentIndex])

        // Load the last used wave type
        dcWaveTypeCB.currentIndex = settingStorage.value("dc_wave_type", 0)
        if (dcWaveTypeCB.currentIndex == 0)
            mBuzzer.SetWaveType("Sine")
        else if (dcWaveTypeCB.currentIndex == 1)
            mBuzzer.SetWaveType("Saw")

        // Load volume
        buzzerVolumeSlider.value = settingStorage.value("buzzer_volume", 50)
        mBuzzer.SetVolume(buzzerVolumeSlider.value)
        
        // Load enable/disable buzzer
        enable_buzzer_dc = settingStorage.value("enable_buzzer_dc", false)
        buzzerEnabledForDC.checked = enable_buzzer_dc

        // Load buzzer threshold
        buzzer_dc_threshold = settingStorage.value("buzzer_dc_threshold", "90")
        buzzerDcThreshold_TF.text = buzzer_dc_threshold

        // Load octave
        buzzerDcOctave_TF.text = settingStorage.value("buzzer_dc_octave", 3)
        mBuzzer.SetOctave(buzzerDcOctave_TF.text)
    }
    
    Component.onDestruction: {
        
    }
    
    ColumnLayout {
        

        Text {
            color: Utility.getAppHexColor("lightText")
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
            text: "Audio Output Device: "
        }
        ComboBox {
            id: audioDevicesDownBox
            Layout.fillWidth: true
            editable: false

            model: ListModel {
                id: audioDevicesDbModel
            }
                    
            onCurrentIndexChanged: {
                settingStorage.setValue("audio_device", currentIndex)
            mBuzzer.SetDeviceByName(audio_devs[currentIndex])
            }
        }
        RowLayout{
            Text {
                color: Utility.getAppHexColor("lightText")
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WordWrap
                text: "General Volume:"
            }
            Slider {
                id: buzzerVolumeSlider
                from: 0
                value: 50
                to: 100
                Layout.fillWidth: true

                onValueChanged: {
                    if(buzzerVolumeSlider.value != buzzer_volume){
                        buzzer_volume = buzzerVolumeSlider.value
                        mBuzzer.SetVolume(buzzer_volume)
                        settingStorage.setValue("buzzer_volume", buzzer_volume)
                    }
                }
            }
        }
        Item {
            Layout.fillWidth: true
            height: 25
            Rectangle {
                id: rect
                anchors.fill: parent
                color: {color = Utility.getAppHexColor("darkAccent")}
                radius: 5

                Text {
                    anchors.centerIn: parent
                    color: {color = Utility.getAppHexColor("lightText")}
                    id: buzzerDcSeparator
                    text: "Duty Cycle Buzzer"
                    font.bold: true
                    font.pointSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
        CheckBox {
            id: buzzerEnabledForDC
            checked: false
            text: qsTr("Enabled Buzzer for Duty Cycle")
            onClicked: {
                enable_buzzer_dc = buzzerEnabledForDC.checked
                settingStorage.setValue("enable_buzzer_dc", enable_buzzer_dc)
            }
        }
        RowLayout{
            Text {
                color: Utility.getAppHexColor("lightText")
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WordWrap
                text: "DC Threshold:"
            }
            TextField {
                id: buzzerDcThreshold_TF
                placeholderText: "0-100"
                text: "90"
                validator: IntValidator{bottom: 0; top: 100;}
                onTextChanged: {
                    settingStorage.setValue("buzzer_dc_threshold", text)
                }
            }
        }
        RowLayout{
            Text {
                color: Utility.getAppHexColor("lightText")
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WordWrap
                text: "Octave:"
            }
            TextField {
                id: buzzerDcOctave_TF
                placeholderText: "0-6"
                text: "3"
                validator: IntValidator{bottom: 0; top: 6;}
                onTextChanged: {
                    settingStorage.setValue("buzzer_dc_octave", buzzerDcOctave_TF.text)
                    mBuzzer.SetOctave(buzzerDcOctave_TF.text)
                }
            }
            Text {
                color: Utility.getAppHexColor("lightText")
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WordWrap
                text: "Note:"
            }
            ComboBox {
                id: buzzerDcNoteCB

                model: ListModel {
                    id: buzzerDcNoteModel
                }
                        
                onCurrentIndexChanged: {
                    settingStorage.setValue("dc_note", currentIndex)
                }
            }
        }
        RowLayout{
            Text {
                color: Utility.getAppHexColor("lightText")
                verticalAlignment: Text.AlignVCenter
                wrapMode: Text.WordWrap
                text: "Wave:"
            }
            ComboBox {
                id: dcWaveTypeCB
                Layout.fillWidth: true
                editable: false

                model: ListModel {
                    id: dcWaveTypeModel
                }
                        
                onCurrentIndexChanged: {
                    settingStorage.setValue("dc_wave_type", currentIndex)
                    if (currentIndex == 0)
                    {
                        mBuzzer.SetWaveType("Sine")
                    }
                    else if (currentIndex == 1)
                    {
                        mBuzzer.SetWaveType("Saw")
                    }
                }
            }
        }

        RowLayout{
            Button {
                text: "Play"
                onClicked: {                
                    mBuzzer.NoteOn(audio_notes[settingStorage.value("dc_note", 0)]) // Include octave and volume
                }
            }
            Button {
                text: "Stop"
                onClicked: {                
                    mBuzzer.NoteOff()
                }
            }
        }
        RowLayout{
            Button {
                text: "Get Error"
                onClicked: {              
                    buzzer_error_text = mBuzzer.GetError()
                }
            }
            Button {
                text: "Clear Error"
                onClicked: {         
                    mBuzzer.ClearError()         
                    buzzer_error_text = ""
                }
            }
        }
        Text {
            color: Utility.getAppHexColor("lightText")
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
            text: buzzer_error_text
        }
    


    }
}
