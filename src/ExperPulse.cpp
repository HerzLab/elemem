#include "ExperPulse.h"
#include "Handler.h"
#include "JSONLines.h"
#include "Popup.h"
#include "StatusPanel.h"

using namespace RC;

namespace CML {
  ExperPulse::ExperPulse(RC::Ptr<Handler> hndl)
    : hndl(hndl) {
    AddToThread(this);
  }

  ExperPulse::~ExperPulse() {
    Stop_Handler();
  }

  void ExperPulse::SetStimProfile_Handler(const StimProfile& new_stim_profile) {
    base_stim_profile = new_stim_profile;

    if (base_stim_profile.size() == 0) {
      Throw_RC_Error("ExperPulse requires at least one stimulation channel.");
    }
  }

  uint64_t ExperPulse::CurrentPulseIntervalMs() const {
    return pulse_intervals_ms[pulse_interval_idx];
  }

  uint32_t ExperPulse::CurrentPairFrequencyHz() const {
    const uint64_t interval_ms = CurrentPulseIntervalMs();
    return uint32_t((1000 + interval_ms/2) / interval_ms);
  }

  uint32_t ExperPulse::CurrentPairDurationUs() const {
    const uint32_t frequency = CurrentPairFrequencyHz();
    return (2000000 + frequency - 1) / frequency;
  }

  double ExperPulse::CurrentActualPulseIntervalMs() const {
    return 1000.0 / CurrentPairFrequencyHz();
  }

  void ExperPulse::ConfigureCurrentInterval() {
    stim_profile.Clear();

    for (size_t i=0; i<base_stim_profile.size(); i++) {
      StimChannel chan = base_stim_profile[i];
      chan.frequency = CurrentPairFrequencyHz();
      chan.duration = CurrentPairDurationUs();
      chan.burst_frac = 1;
      chan.burst_slow_freq = 0;
      stim_profile += chan;
    }

    hndl->stim_worker.ConfigureStimulation(stim_profile);
  }

  void ExperPulse::Start_Handler() {
    if (base_stim_profile.size() == 0) {
      Throw_RC_Error("ExperPulse cannot start without a stimulation profile.");
    }

    BeAllocatedTimer();
    next_event_time = 0;
    pulse_pair_count = 0;
    pulse_interval_idx = 0;
    pulse_interval_trial = 0;
    pulse_interval_repeat = 0;
    ConfigureCurrentInterval();

    if (!ConfirmWin("Pulse experiment will run for 6 min. 0 sec.",
          "Session Duration")) {
      hndl->StopExperiment();
      return;
    }

    exp_start = RC::Time::Get();

    JSONFile startlog = MakeResp("START");
    RC::Data1D<uint64_t> pulse_intervals_log(pulse_intervals_ms.size());
    for (size_t i=0; i<pulse_intervals_ms.size(); i++) {
      pulse_intervals_log[i] = pulse_intervals_ms[i];
    }
    startlog.Set(pulse_intervals_log, "data", "pulse_intervals_ms");
    startlog.Set(trials_per_interval, "data", "trials_per_interval");
    startlog.Set(interval_repeats, "data", "interval_repeats");
    startlog.Set(pair_period_ms, "data", "pair_period_ms");
    startlog.Set(experiment_duration_ms, "data", "experiment_duration_ms");
    hndl->event_log.Log(startlog.Line());

    RunEvent();
  }

  void ExperPulse::Stop_Handler() {
    if (timer.IsSet()) {
      timer->stop();
    }
  }

  void ExperPulse::InternalStop() {
    JSONFile stoplog = MakeResp("EXIT");
    stoplog.Set(pulse_pair_count, "data", "pulse_pair_count");
    hndl->event_log.Log(stoplog.Line());

    Stop_Handler();
    hndl->StopExperiment();
  }

  void ExperPulse::DoPulsePair() {
    JSONFile pulse_event = MakeResp("PULSE_PAIR");
    pulse_event.Set(pulse_pair_count, "data", "pair_index");
    pulse_event.Set(pulse_interval_repeat, "data", "interval_repeat");
    pulse_event.Set(pulse_interval_idx, "data", "interval_index");
    pulse_event.Set(pulse_interval_trial, "data", "interval_trial");
    pulse_event.Set(CurrentPulseIntervalMs(), "data", "requested_pulse_interval_ms");
    pulse_event.Set(CurrentActualPulseIntervalMs(), "data", "actual_pulse_interval_ms");
    pulse_event.Set(CurrentPairFrequencyHz(), "data", "frequency_Hz");
    pulse_event.Set(CurrentPairDurationUs(), "data", "duration_us");
    hndl->event_log.Log(pulse_event.Line());

    if (status_panel.IsSet()) {
      status_panel->SetEvent("PULSE");
    }
    hndl->stim_worker.Stimulate();
    pulse_pair_count++;
    pulse_interval_trial++;

    if (pulse_interval_trial >= trials_per_interval) {
      pulse_interval_trial = 0;
      pulse_interval_idx++;

      if (pulse_interval_idx >= pulse_intervals_ms.size()) {
        pulse_interval_idx = 0;
        pulse_interval_repeat++;
      }

      if (pulse_pair_count < total_pair_count) {
        ConfigureCurrentInterval();
      }
    }
  }

  void ExperPulse::RunEvent() {
    if (pulse_pair_count >= total_pair_count) {
      InternalStop();
      return;
    }

    DoPulsePair();
    next_event_time += pair_period_ms;
    TriggerAt(next_event_time);
  }

  void ExperPulse::TriggerAt(uint64_t target_ms) {
    f64 cur_time = RC::Time::Get();
    uint64_t current_time_ms = uint64_t(1000*(cur_time - exp_start)+0.5);

    uint64_t delay;
    if (target_ms <= current_time_ms) {
      delay = 0;
    }
    else {
      delay = target_ms - current_time_ms;
    }

    timer->start(delay);
  }

  void ExperPulse::BeAllocatedTimer() {
    if (timer.IsNull()) {
      timer = new QTimer();
      timer->setTimerType(Qt::PreciseTimer);
      timer->setSingleShot(true);

      QObject::connect(timer.Raw(), &QTimer::timeout, this,
                     &ExperPulse::RunEvent);
    }
  }
}
