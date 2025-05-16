/*
 *  Copyright 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc_fakeAudioCaptureModule.hpp"


#include "rtc_base/checks.h"
#include "rtc_base/location.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/thread.h"
#include "rtc_base/time_utils.h"

// Audio sample value that is high enough that it doesn't occur naturally when
// frames are being faked. E.g. NetEq will not generate this large sample value
// unless it has received an audio frame containing a sample of this value.
// Even simpler buffers would likely just contain audio sample values of 0.
static const int kHighSampleValue = 10000;

// Constants here are derived by running VoE using a real ADM.
// The constants correspond to 10ms of mono audio at 44kHz.
static const int kTimePerFrameMs = 10;
static const uint8_t kNumberOfChannels = 1;
static const int kSamplesPerSecond = 44000;
static const int kTotalDelayMs = 0;
static const int kClockDriftMs = 0;
static const uint32_t kMaxVolume = 14392;

enum {
  MSG_START_PROCESS,
  MSG_RUN_PROCESS,
};

de::stream_webrtc::CFakeAudioCaptureModule::CFakeAudioCaptureModule()
    : audio_callback_(nullptr),
      recording_(false),
      playing_(false),
      play_is_initialized_(false),
      rec_is_initialized_(false),
      current_mic_level_(kMaxVolume),
      started_(false),
      next_frame_time_(0),
      frames_received_(0) {
  process_thread_checker_.Detach();
}

de::stream_webrtc::CFakeAudioCaptureModule::~CFakeAudioCaptureModule() {
  if (process_thread_) {
    process_thread_->Stop();
  }
}

rtc::scoped_refptr<de::stream_webrtc::CFakeAudioCaptureModule> de::stream_webrtc::CFakeAudioCaptureModule::Create() {
  auto capture_module = rtc::make_ref_counted<de::stream_webrtc::CFakeAudioCaptureModule>();
  if (!capture_module->Initialize()) {
    return 0; //nullptr;
  }
  return capture_module;
}

int de::stream_webrtc::CFakeAudioCaptureModule::frames_received() const {
  webrtc::MutexLock lock(&mutex_);
  return frames_received_;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::ActiveAudioLayer(
    AudioLayer* /*audio_layer*/) const {
  RTC_NOTREACHED();
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::RegisterAudioCallback(
    webrtc::AudioTransport* audio_callback) {
 return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::Init() {
  // Initialize is called by the factory method. Safe to ignore this Init call.
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::Terminate() {
  // Clean up in the destructor. No action here, just success.
  return 0;
}

bool de::stream_webrtc::CFakeAudioCaptureModule::Initialized() const {
  RTC_NOTREACHED();
  return 0;
}

int16_t de::stream_webrtc::CFakeAudioCaptureModule::PlayoutDevices() {
  RTC_NOTREACHED();
  return 0;
}

int16_t de::stream_webrtc::CFakeAudioCaptureModule::RecordingDevices() {
  RTC_NOTREACHED();
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::PlayoutDeviceName(
    uint16_t /*index*/,
    char /*name*/[webrtc::kAdmMaxDeviceNameSize],
    char /*guid*/[webrtc::kAdmMaxGuidSize]) {
  RTC_NOTREACHED();
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::RecordingDeviceName(
    uint16_t /*index*/,
    char /*name*/[webrtc::kAdmMaxDeviceNameSize],
    char /*guid*/[webrtc::kAdmMaxGuidSize]) {
  RTC_NOTREACHED();
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::SetPlayoutDevice(uint16_t /*index*/) {
  // No playout device, just playing from file. Return success.
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::SetPlayoutDevice(WindowsDeviceType /*device*/) {
  if (play_is_initialized_) {
    return -1;
  }
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::SetRecordingDevice(uint16_t /*index*/) {
  // No recording device, just dropping audio. Return success.
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::SetRecordingDevice(
    WindowsDeviceType /*device*/) {
  if (rec_is_initialized_) {
    return -1;
  }
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::PlayoutIsAvailable(bool* /*available*/) {
  RTC_NOTREACHED();
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::InitPlayout() {
  play_is_initialized_ = true;
  return 0;
}

bool de::stream_webrtc::CFakeAudioCaptureModule::PlayoutIsInitialized() const {
  return play_is_initialized_;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::RecordingIsAvailable(bool* /*available*/) {
  RTC_NOTREACHED();
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::InitRecording() {
  return 0;
}

bool de::stream_webrtc::CFakeAudioCaptureModule::RecordingIsInitialized() const {
  return rec_is_initialized_;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::StartPlayout() {
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::StopPlayout() {
  return 0;
}

bool de::stream_webrtc::CFakeAudioCaptureModule::Playing() const {
  return playing_;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::StartRecording() {
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::StopRecording() {
  return 0;
}

bool de::stream_webrtc::CFakeAudioCaptureModule::Recording() const {
  return recording_;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::InitSpeaker() {
  // No speaker, just playing from file. Return success.
  return 0;
}

bool de::stream_webrtc::CFakeAudioCaptureModule::SpeakerIsInitialized() const {
  RTC_NOTREACHED();
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::InitMicrophone() {
  // No microphone, just playing from file. Return success.
  return 0;
}

bool de::stream_webrtc::CFakeAudioCaptureModule::MicrophoneIsInitialized() const {
  RTC_NOTREACHED();
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::SpeakerVolumeIsAvailable(bool* /*available*/) {
  RTC_NOTREACHED();
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::SetSpeakerVolume(uint32_t /*volume*/) {
  RTC_NOTREACHED();
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::SpeakerVolume(uint32_t* /*volume*/) const {
  RTC_NOTREACHED();
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::MaxSpeakerVolume(
    uint32_t* /*max_volume*/) const {
  RTC_NOTREACHED();
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::MinSpeakerVolume(
    uint32_t* /*min_volume*/) const {
  RTC_NOTREACHED();
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::MicrophoneVolumeIsAvailable(
    bool* /*available*/) {
  RTC_NOTREACHED();
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::SetMicrophoneVolume(uint32_t volume) {
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::MicrophoneVolume(uint32_t* volume) const {
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::MaxMicrophoneVolume(
    uint32_t* max_volume) const {
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::MinMicrophoneVolume(
    uint32_t* /*min_volume*/) const {
   return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::SpeakerMuteIsAvailable(bool* /*available*/) {
  RTC_NOTREACHED();
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::SetSpeakerMute(bool /*enable*/) {
  RTC_NOTREACHED();
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::SpeakerMute(bool* /*enabled*/) const {
  RTC_NOTREACHED();
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::MicrophoneMuteIsAvailable(bool* /*available*/) {
  RTC_NOTREACHED();
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::SetMicrophoneMute(bool /*enable*/) {
  RTC_NOTREACHED();
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::MicrophoneMute(bool* /*enabled*/) const {
  RTC_NOTREACHED();
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::StereoPlayoutIsAvailable(
    bool* available) const {
  // No recording device, just dropping audio. Stereo can be dropped just
  // as easily as mono.
  *available = true;
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::SetStereoPlayout(bool /*enable*/) {
  // No recording device, just dropping audio. Stereo can be dropped just
  // as easily as mono.
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::StereoPlayout(bool* /*enabled*/) const {
  RTC_NOTREACHED();
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::StereoRecordingIsAvailable(
    bool* available) const {
  // Keep thing simple. No stereo recording.
  *available = false;
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::SetStereoRecording(bool enable) {
  if (!enable) {
    return 0;
  }
  return -1;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::StereoRecording(bool* /*enabled*/) const {
  RTC_NOTREACHED();
  return 0;
}

int32_t de::stream_webrtc::CFakeAudioCaptureModule::PlayoutDelay(uint16_t* delay_ms) const {
  // No delay since audio frames are dropped.

  return 0;
}

void de::stream_webrtc::CFakeAudioCaptureModule::OnMessage(rtc::Message* msg) {

}

bool de::stream_webrtc::CFakeAudioCaptureModule::Initialize() {
 
  return true;
}

void de::stream_webrtc::CFakeAudioCaptureModule::SetSendBuffer(int value) {

}

void de::stream_webrtc::CFakeAudioCaptureModule::ResetRecBuffer() {
}

bool de::stream_webrtc::CFakeAudioCaptureModule::CheckRecBuffer(int value) {
  
  return false;
}

bool de::stream_webrtc::CFakeAudioCaptureModule::ShouldStartProcessing() {
  return recording_ || playing_;
}

void de::stream_webrtc::CFakeAudioCaptureModule::UpdateProcessing(bool start) {
  
}

void de::stream_webrtc::CFakeAudioCaptureModule::StartProcessP() {
 
}

void de::stream_webrtc::CFakeAudioCaptureModule::ProcessFrameP() {
  
}

void de::stream_webrtc::CFakeAudioCaptureModule::ReceiveFrameP() {
  
}

void de::stream_webrtc::CFakeAudioCaptureModule::SendFrameP() {
 
}
