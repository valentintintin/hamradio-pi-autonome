using System.Reactive.Linq;
using Monitor.Extensions;
using Monitor.Models;
using Monitor.Services;
using SunCalcNet;
using SunCalcNet.Model;

namespace Monitor.Workers;

public class BatteryApp : AWorker
{
    public static readonly ConfigEntity<bool> NightEnabled = new("night/enabled", true);
    public static readonly ConfigEntity<bool> NightTurnOn = new("night/turn_on", true);
    public static readonly ConfigEntity<int> NightLimitVoltage = new("night/limit_voltage", true, 11600);
    public static readonly ConfigEntity<TimeSpan> NightTimeOff = new("night/time_off", true, TimeSpan.FromMinutes(60));
    public static readonly ConfigEntity<bool> NightUseSun = new("night/use_sun", true);
    
    public static readonly ConfigEntity<bool> LowBatteryEnabled = new("low_battery/enabled", true);
    public static readonly ConfigEntity<int> LowBatteryVoltage = new("low_battery/voltage", true, 11600);
    public static readonly ConfigEntity<TimeSpan> LowBatteryTimeOff = new("low_battery/time_off", true, TimeSpan.FromMinutes(30));

    private readonly (double latitude, double longitude, int altitude) _position;
    private readonly SystemService _systemService;
    
    public BatteryApp(ILogger<BatteryApp> logger, IServiceProvider serviceProvider, IConfiguration configuration) 
        : base(logger, serviceProvider)
    {
        EntitiesManagerService.Add(LowBatteryEnabled);
        EntitiesManagerService.Add(LowBatteryVoltage);
        EntitiesManagerService.Add(LowBatteryTimeOff);
        
        var configurationSection = configuration.GetSection("Position");

        _position = (
            configurationSection.GetValueOrThrow<double>("Latitude"),
            configurationSection.GetValueOrThrow<double>("Longitude"),
            configurationSection.GetValueOrThrow<int>("Altitude")
        );

        EntitiesManagerService.Add(NightEnabled);
        EntitiesManagerService.Add(NightTurnOn);
        EntitiesManagerService.Add(NightTimeOff);
        EntitiesManagerService.Add(NightUseSun);
        EntitiesManagerService.Add(NightLimitVoltage);
        
        _systemService = Services.GetRequiredService<SystemService>();
    }

    protected override Task Start(CancellationToken cancellationToken)
    {
        AddDisposable(EntitiesManagerService.Entities.BatteryVoltage.ValueChanges()
            .Select(v => v.value)
            .Buffer(TimeSpan.FromMinutes(5))
            .Select(s => s.Any() ? s.Average() : int.MaxValue)
            .Where(_ => LowBatteryEnabled.IsTrue())
            .Where(_ => NightEnabled.IsFalse() || EntitiesManagerService.Entities.SolarIsDay.IsTrue()) // Only during the day of without night management
            .Do(s => Logger.LogDebug("Average Battery Voltage : {averageVoltage}", s))
            .Where(s => s < LowBatteryVoltage.Value)
            .Subscribe(s =>
            {
                Logger.LogInformation("Battery is too low so sleep. {batteryVoltage} < {lowVoltage}", s, LowBatteryVoltage.Value);

                _systemService.AskForShutdown(LowBatteryTimeOff.Value);
            }));
        
        AddDisposable(EntitiesManagerService.Entities.SolarIsDay.IsFalseAsync()
            .Where(_ => NightEnabled.IsTrue())
            .Subscribe(_ =>
            {
                DoNight();
            })
        );
        
        return Task.CompletedTask;
    }
    
    private void DoNight()
    {
        if (_systemService.IsShutdownAsked())
        {
            Logger.LogDebug("Night but time sleep already set");
            
            return;
        }

        var durationSleep = NightTimeOff.Value;
        var shouldSwitchOnDuringNight = NightTurnOn.IsTrue();
        var batteryVoltage = EntitiesManagerService.Entities.BatteryVoltage.Value;
        var limitVoltage = NightLimitVoltage.Value;
        
        // Ask for today (and tomorrow) sun rising end (morning)
        var sunRisingDateTime = SunCalc.GetSunPhases(DateTime.UtcNow, _position.latitude, _position.longitude, _position.altitude)
            .Concat(SunCalc.GetSunPhases(DateTime.UtcNow.AddDays(1), _position.latitude, _position.longitude, _position.altitude))
            .First(e => e.Name.Value == SunPhaseName.SunriseEnd.Value && e.PhaseTime > DateTime.UtcNow)
            .PhaseTime;
        
        if (batteryVoltage <= limitVoltage)
        {
            Logger.LogInformation("Night detected and Battery Voltage is {batteryVoltage} so below {limitVoltage}", batteryVoltage, limitVoltage);
            
            shouldSwitchOnDuringNight = false;
        }
        
        if (NightUseSun.IsTrue())
        {
            var durationBeforeSunRinsing = sunRisingDateTime - DateTime.UtcNow;
            Logger.LogDebug("Sun rising is in {duration} ==> {sunRisingDateTime}", durationBeforeSunRinsing, sunRisingDateTime);

            if (DateTime.UtcNow < sunRisingDateTime)
            {
                if (shouldSwitchOnDuringNight)
                {
                    if (durationSleep > durationBeforeSunRinsing)
                    {
                        Logger.LogInformation(
                            "Sleep too long and we will miss sunrise so sleep to sunrise instead of {duration}",
                            durationSleep);

                        durationSleep = durationBeforeSunRinsing;
                    }
                }
                else
                {
                    Logger.LogDebug("We do not turn on during night so sleep to sun rising");

                    durationSleep = durationBeforeSunRinsing; // Instead of using our default settings
                }
            }
        }
        else
        {
            Logger.LogDebug("We do not use sun");
            
            if (!shouldSwitchOnDuringNight)
            {
                Logger.LogDebug("We do not turn on during night");
                
                durationSleep = TimeSpan.FromHours(10);
            }
        }

        _systemService.AskForShutdown(durationSleep);
    }
}