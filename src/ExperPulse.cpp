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
    stim_profile.Clear();

    for (size_t i=0; i<new_stim_profile.size(); i++) {
      StimChannel chan = new_stim_profile[i];
      chan.frequency = pair_frequency_hz;
      chan.duration = pair_duration_us;
      chan.burst_frac = 1;
      chan.burst_slow_freq = 0;
      stim_profile += chan;
    }

    if (stim_profile.size() == 0) {
      Throw_RC_Error("ExperPulse requires at least one stimulation channel.");
    }

    hndl->stim_worker.ConfigureStimulation(stim_profile);
  }

  void ExperPulse::Start_Handler() {
    if (stim_profile.size() == 0) {
      Throw_RC_Error("ExperPulse cannot start without a stimulation profile.");
    }

    BeAllocatedTimer();
    next_event_time = 0;
    pulse_pair_count = 0;

    if (!ConfirmWin("Pulse experiment will run for 5 min. 0 sec.",
          "Session Duration")) {
      hndl->StopExperiment();
      return;
    }

    exp_start = RC::Time::Get();

    JSONFile startlog = MakeResp("START");
    startlog.Set(pulse_interval_ms, "data", "pulse_interval_ms");
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
    pulse_event.Set(pulse_interval_ms, "data", "pulse_interval_ms");
    hndl->event_log.Log(pulse_event.Line());

    if (status_panel.IsSet()) {
      status_panel->SetEvent("PULSE");
    }
    hndl->stim_worker.Stimulate();
    pulse_pair_count++;
  }

  void ExperPulse::RunEvent() {
    if (next_event_time >= experiment_duration_ms) {
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
