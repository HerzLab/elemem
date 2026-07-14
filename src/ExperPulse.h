#ifndef EXPERPULSE_H
#define EXPERPULSE_H

#include "RC/APtr.h"
#include "RC/Data1D.h"
#include "RC/Ptr.h"
#include "RC/RStr.h"
#include "RCqt/Worker.h"
#include "StimInterface.h"
#include <array>
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
    void ConfigureCurrentInterval();
    uint64_t CurrentPulseIntervalMs() const;
    uint32_t CurrentPairFrequencyHz() const;
    uint32_t CurrentPairDurationUs() const;
    double CurrentActualPulseIntervalMs() const;

    protected slots:
    void RunEvent();

    protected:
    void TriggerAt(uint64_t target_ms);
    void BeAllocatedTimer();

    static constexpr uint64_t pair_period_ms = 1000;
    static constexpr size_t trials_per_interval = 10;
    static constexpr size_t interval_repeats = 6;
    static constexpr std::array<uint64_t, 6> pulse_intervals_ms =
      {5, 15, 30, 50, 100, 200};
    static constexpr size_t total_pair_count =
      pulse_intervals_ms.size() * trials_per_interval * interval_repeats;
    static constexpr uint64_t experiment_duration_ms =
      total_pair_count * pair_period_ms;

    RC::Ptr<Handler> hndl;
    RC::Ptr<StatusPanel> status_panel;
    RC::APtr<QTimer> timer;
    StimProfile base_stim_profile;
    StimProfile stim_profile;
    uint64_t next_event_time = 0;
    size_t pulse_pair_count = 0;
    size_t pulse_interval_idx = 0;
    size_t pulse_interval_trial = 0;
    size_t pulse_interval_repeat = 0;
    f64 exp_start = 0;
  };
}

#endif // EXPERPULSE_H
