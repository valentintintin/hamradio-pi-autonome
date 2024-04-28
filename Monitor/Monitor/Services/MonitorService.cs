using AprsSharp.AprsParser;
using Microsoft.EntityFrameworkCore;
using Monitor.Context;
using Monitor.Context.Entities;
using Monitor.Extensions;
using Monitor.Models;
using Monitor.Models.SerialMessages;

namespace Monitor.Services;

public class MonitorService(
    ILogger<MonitorService> logger,
    IDbContextFactory<DataContext> contextFactory,
    SystemService systemService)
    : AService(logger)
{
    public static readonly MonitorState State = new();

    public async Task UpdateStateFromMessage(Message message)
    {
        var context = await contextFactory.CreateDbContextAsync();

        State.LastMessagesReceived.Add(message);
        
        switch (message)
        {
            case McuSystemData systemData:
                Logger.LogDebug("New system data received : {message}", systemData);
                
                State.McuSystem = systemData;
                
                EntitiesManagerService.Entities.McuStatus.SetValue(systemData.State, true);
                EntitiesManagerService.Entities.StatusBoxOpened.SetValue(systemData.BoxOpened, true);
                EntitiesManagerService.Entities.FeatureWatchdogSafetyEnabled.SetValue(systemData.WatchdogSafetyEnabled, true);
                EntitiesManagerService.Entities.FeatureAprsDigipeaterEnabled.SetValue(systemData.AprsDigipeaterEnabled, true);
                EntitiesManagerService.Entities.FeatureAprsTelemetryEnabled.SetValue(systemData.AprsTelemetryEnabled, true);
                EntitiesManagerService.Entities.FeatureAprsPositionEnabled.SetValue(systemData.AprsPositionEnabled, true);
                EntitiesManagerService.Entities.FeatureSleepEnabled.SetValue(systemData.Sleep, true);
                EntitiesManagerService.Entities.TemperatureRtc.SetValue(systemData.TemperatureRtc, true);
                EntitiesManagerService.Entities.McuUptime.SetValue(systemData.UptimeTimeSpan, true);
                
                systemService.SetTime(State.McuSystem.DateTime.DateTime);
                break;
            case WeatherData weatherData:
                Logger.LogDebug("New weather data received : {message}", weatherData);

                State.Weather = weatherData;
                
                EntitiesManagerService.Entities.WeatherTemperature.SetValue(weatherData.Temperature, true);
                EntitiesManagerService.Entities.WeatherHumidity.SetValue(weatherData.Humidity, true);
                EntitiesManagerService.Entities.WeatherPressure.SetValue(weatherData.Pressure, true);

                context.Add(new Weather
                {
                    Temperature = weatherData.Temperature,
                    Humidity = weatherData.Humidity,
                    Pressure = weatherData.Pressure
                });
                await context.SaveChangesAsync();
                break;
            case MpptData mpptData:
                Logger.LogDebug("New MPPT data received : {message}", mpptData);

                State.Mppt = mpptData;
                
                EntitiesManagerService.Entities.BatteryVoltage.SetValue(mpptData.BatteryVoltage, true);
                EntitiesManagerService.Entities.BatteryCurrent.SetValue(mpptData.BatteryCurrent, true);
                EntitiesManagerService.Entities.SolarVoltage.SetValue(mpptData.SolarVoltage, true);
                EntitiesManagerService.Entities.SolarCurrent.SetValue(mpptData.SolarCurrent, true);
                EntitiesManagerService.Entities.MpptChargeCurrent.SetValue(mpptData.CurrentCharge, true);
                EntitiesManagerService.Entities.MpptStatus.SetValue(mpptData.StatusString, true);
                EntitiesManagerService.Entities.SolarIsDay.SetValue(!mpptData.Night, true);
                EntitiesManagerService.Entities.MpptAlertShutdown.SetValue(mpptData.Alert, true);
                EntitiesManagerService.Entities.MpptPowerEnabled.SetValue(mpptData.PowerEnabled, true);
                EntitiesManagerService.Entities.WatchdogEnabled.SetValue(mpptData.WatchdogEnabled, true);
                EntitiesManagerService.Entities.WatchdogCounter.SetValue(TimeSpan.FromSeconds(mpptData.WatchdogCounter), true);
                EntitiesManagerService.Entities.WatchdogPowerOffTime.SetValue(TimeSpan.FromSeconds(mpptData.WatchdogPowerOffTime), true);
                EntitiesManagerService.Entities.MpptPowerOffVoltage.SetValue(mpptData.PowerOffVoltage, true);
                EntitiesManagerService.Entities.MpptPowerOnVoltage.SetValue(mpptData.PowerOnVoltage, true);
                EntitiesManagerService.Entities.MpptTemperature.SetValue(mpptData.Temperature, true);

                context.Add(new Mppt
                {
                    BatteryVoltage = mpptData.BatteryVoltage,
                    BatteryCurrent = mpptData.BatteryCurrent,
                    SolarVoltage = mpptData.SolarVoltage,
                    SolarCurrent = mpptData.SolarCurrent,
                    CurrentCharge = mpptData.CurrentCharge,
                    Status = mpptData.Status,
                    StatusString = mpptData.StatusString,
                    Night = mpptData.Night,
                    Alert = mpptData.Alert,
                    WatchdogEnabled = mpptData.WatchdogEnabled,
                    WatchdogPowerOffTime = mpptData.WatchdogPowerOffTime,
                    Temperature = mpptData.Temperature
                });
                await context.SaveChangesAsync();
                break;
            case GpioData gpioData:
                Logger.LogDebug("New GPIO data received : {message}", gpioData);

                State.Gpio = gpioData;
                
                EntitiesManagerService.Entities.GpioWifi.SetValue(gpioData.Wifi, true);
                EntitiesManagerService.Entities.GpioNpr.SetValue(gpioData.Npr, true);
                EntitiesManagerService.Entities.GpioBoxLdr.SetValue(gpioData.Ldr, true);

                context.Add(new Context.Entities.System
                {
                    Npr = gpioData.Npr,
                    Wifi = gpioData.Wifi,
                    Uptime = (long)EntitiesManagerService.Entities.SystemUptime.Value.TotalSeconds,
                    BoxOpened = State.McuSystem.BoxOpened,
                    McuUptime = State.McuSystem.Uptime,
                    TemperatureRtc = State.McuSystem.TemperatureRtc
                });
                await context.SaveChangesAsync();
                break;
            case LoraData loraData:
                Logger.LogDebug("New LoRa data received : {message}", loraData);
                
                if (loraData.IsTx)
                {
                    State.Lora.LastTx.Add(loraData);
                
                    EntitiesManagerService.Entities.LoraTxPayload.SetValue(loraData.Payload, true);
                }
                else
                {
                    State.Lora.LastRx.Add(loraData);
                
                    EntitiesManagerService.Entities.LoraRxPayload.SetValue(loraData.Payload, true);
                }

                string? sender = null;
                
                try
                {
                    var packet = loraData.Payload.ToAprsPacket();
                    sender = packet.Sender;
                }
                catch (Exception e)
                {
                    Logger.LogWarning(e, "LoRa APRS frame received not decodable : {payload}", loraData.Payload);
                }

                context.Add(new LoRa
                {
                    Sender = sender,
                    Frame = loraData.Payload,
                    IsTx = loraData.IsTx,
                    IsMeshtastic = false
                });
                await context.SaveChangesAsync();
                break;
        }
    }

    public void UpdateSystemState(SystemState? systemState)
    {
        Logger.LogInformation("New system info received");
        
        State.System = systemState;

        if (State.System != null)
        {
            EntitiesManagerService.Entities.SystemUptime.SetValue(State.System.UptimeTimeSpan, true);
            EntitiesManagerService.Entities.SystemCpu.SetValue(State.System.Cpu, true);
            EntitiesManagerService.Entities.SystemRam.SetValue(State.System.Ram, true);
            EntitiesManagerService.Entities.SystemDisk.SetValue(State.System.Disk, true);
        }
    }
}