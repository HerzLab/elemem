#ifndef EXPERPULSE_H
#define EXPERPULSE_H

#include "RC/APtr.h"
#include "RC/Data1D.h"
#include "RC/Ptr.h"
#include "RC/RStr.h"
#include "RCqt/Worker.h"
#include "StimInterface.h"
#include <QTimer>

namespace CML {
  class Handler;
  class StatusPanel;

  class ExperPulse : public RCqt::WorkerThread, public QObject {
    public:
    ExperPulse(RC::Ptr<Handler> hndl);
    ~ExperPulse();

    ExperPulse(const ExperPulse&) = delete;
    ExperPulse& operator=(const ExperPulse&) = delete;

    RCqt::TaskCaller<const StimProfile> SetStimProfile =
      TaskHandler(ExperPulse::SetStimProfile_Handler);

    RCqt::TaskCaller<const RC::Ptr<StatusPanel>> SetStatusPanel =
      TaskHandler(ExperPulse::SetStatusPanel_Handler);

    RCqt::TaskCaller<> Start =
      TaskHandler(ExperPulse::Start_Handler);

    RCqt::TaskBlocker<> Stop =
      TaskHandler(ExperPulse::Stop_Handler);

    protected:
    void SetStimProfile_Handler(const StimProfile& new_stim_profile);
    void SetStatusPanel_Handler(const RC::Ptr<StatusPanel>& set_panel) {
      status_panel = set_panel;
    }

    void Start_Handler();
    void Stop_Handler();
    void InternalStop();
    void DoPulsePair();

    protected slots:
    void RunEvent();

    protected:
    void TriggerAt(uint64_t target_ms);
    void BeAllocatedTimer();

    static constexpr uint64_t pulse_interval_ms = 30;
    static constexpr uint64_t pair_period_ms = 1000;
    static constexpr uint64_t experiment_duration_ms = 5 * 60 * 1000;
    static constexpr uint32_t pair_frequency_hz = 33;
    static constexpr uint32_t pair_duration_us = 61000;

    RC::Ptr<Handler> hndl;
    RC::Ptr<StatusPanel> status_panel;
    RC::APtr<QTimer> timer;
    StimProfile stim_profile;
    uint64_t next_event_time = 0;
    size_t pulse_pair_count = 0;
    f64 exp_start = 0;
  };
}

#endif // EXPERPULSE_H
