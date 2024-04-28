using System.Reactive.Linq;
using Monitor.Services;

namespace Monitor.Workers;

public class McuFeatureApp : AWorker
{
    private readonly SerialMessageService _serialMessageService;

    public McuFeatureApp(ILogger<McuFeatureApp> logger, IServiceProvider serviceProvider) 
        : base(logger, serviceProvider)
    {
        _serialMessageService = Services.GetRequiredService<SerialMessageService>();
    }

    protected override Task Start()
    {
        AddDisposable(EntitiesManagerService.Entities.FeatureWatchdogSafetyEnabled.ValueToChange()
            .Sample(TimeSpan.FromSeconds(1))
            .Do(v => Logger.LogDebug("Watchdog Safety => {value}", v))
            .Select(v => v.value)
            .Subscribe(_serialMessageService.SetWatchdogSafety)
        );
        
        AddDisposable(EntitiesManagerService.Entities.FeatureAprsDigipeaterEnabled.ValueToChange()
            .Sample(TimeSpan.FromSeconds(1))
            .Do(v => Logger.LogDebug("APRS DigiPeater => {value}", v))
            .Select(v => v.value)
            .Subscribe(_serialMessageService.SetAprsDigipeater)
        );
        
        AddDisposable(EntitiesManagerService.Entities.FeatureAprsTelemetryEnabled.ValueToChange()
            .Sample(TimeSpan.FromSeconds(1))
            .Do(v => Logger.LogDebug("APRS Telemetry => {value}", v))
            .Select(v => v.value)
            .Subscribe(_serialMessageService.SetAprsTelemetry)
        );
        
        AddDisposable(EntitiesManagerService.Entities.FeatureAprsPositionEnabled.ValueToChange()
            .Sample(TimeSpan.FromSeconds(1))
            .Do(v => Logger.LogDebug("APRS Position => {value}", v))
            .Select(v => v.value)
            .Subscribe(_serialMessageService.SetAprsPosition)
        );

        AddDisposable(EntitiesManagerService.Entities.FeatureSleepEnabled.ValueToChange()
            .Sample(TimeSpan.FromSeconds(1))
            .Do(v => Logger.LogDebug("Sleep => {value}", v))
            .Select(v => v.value)
            .Subscribe(_serialMessageService.SetSleep)
        );

        return Task.CompletedTask;
    }
}