using System.Reactive.Concurrency;
using Meshtastic.Connections;
using Meshtastic.Data;
using Meshtastic.Data.MessageFactories;
using Meshtastic.Protobufs;
using Monitor.Extensions;
using Monitor.Services;
using Position = Meshtastic.Protobufs.Position;

namespace Monitor.Workers;

public class MeshtasticApp : AEnabledWorker
{
    private readonly string _path;
    private readonly int _speed;
    private readonly bool _simulate;
    private readonly double _latitude, _longitude;
    private readonly int _altitude;
    private readonly ToRadioMessageFactory _toRadioMessageFactory = new();
    private DeviceConnection? _deviceConnection;
    
    public MeshtasticApp(ILogger<MeshtasticApp> logger, IServiceProvider serviceProvider,
        IConfiguration configuration) : base(logger, serviceProvider)
    {
        RetryDuration = TimeSpan.FromSeconds(30);
            
        var configurationSection = configuration.GetSection("SerialPortMeshtastic");
        _path = configurationSection.GetValueOrThrow<string>("Path");
        _speed = configurationSection.GetValue("Speed", 38400);
        _simulate = configurationSection.GetValue("Simulate", false);
        
        var positionSection = configuration.GetSection("Position");
        _latitude = positionSection.GetValueOrThrow<double>("Latitude");
        _longitude = positionSection.GetValueOrThrow<double>("Longitude");
        _altitude = positionSection.GetValueOrThrow<int>("Altitude");
    }

    protected override async Task Start()
    {
        if (_simulate)
        {
            _deviceConnection = new SimulatedConnection(Logger);
        }
        else
        {
            _deviceConnection = new SerialConnection(Logger, _path, _speed);
        }
        await SendWeather();

        AddDisposable(
            Scheduler.SchedulePeriodic(TimeSpan.FromHours(1), async () =>
            {
                await SendWeather();
            })
        );
        
        AddDisposable(
            Scheduler.SchedulePeriodic(TimeSpan.FromDays(1), async () =>
            {
                await SetPosition();
            })
        );
        
        // await SetPosition();
    }

    protected override Task Stop()
    {
        _deviceConnection?.Disconnect();
        
        return base.Stop();
    }

    private async Task<DeviceStateContainer> GetContainer()
    {
        if (_deviceConnection == null)
        {
            throw new Exception("No connection");
        }
        
        var wantConfig = new ToRadioMessageFactory().CreateWantConfigMessage();
        return await _deviceConnection.WriteToRadio(wantConfig, async (_, _) =>
        {
            await Task.Delay(500);
            return true;
        });
    }

    private async Task SendWeather()
    {
        if (_deviceConnection == null)
        {
            throw new Exception("No connection");
        }
        
        var textMessageFactory = new TextMessageFactory(await GetContainer());
        var textMessage = textMessageFactory.CreateTextMessagePacket($"{EntitiesManagerService.Entities.WeatherTemperature.Value}°C {EntitiesManagerService.Entities.WeatherHumidity.Value}% {EntitiesManagerService.Entities.WeatherPressure.Value}hPa {EntitiesManagerService.Entities.MpptChargeCurrent.Value}mA {EntitiesManagerService.Entities.BatteryVoltage.Value}mV");
        textMessage.WantAck = false;
        
        Logger.LogInformation("Sending telemetry message to primary channel");

        await _deviceConnection.WriteToRadio(_toRadioMessageFactory.CreateMeshPacketMessage(textMessage), 
            (_, _) =>
            {
                Logger.LogInformation("Sending telemetry message OK");
                return Task.FromResult(true);
            });
    }

    private async Task SetPosition()
    {
        if (_deviceConnection == null)
        {
            throw new Exception("No connection");
        }
        
        var adminMessageFactory = new AdminMessageFactory(await GetContainer());

        decimal divisor = new(1e-7);
        var position = new Position
        {
            LatitudeI = decimal.ToInt32((decimal)(_latitude / (double)divisor)),
            LongitudeI = decimal.ToInt32((decimal)(_longitude / (double)divisor)),
            Altitude = _altitude
        };

        var adminMessage = adminMessageFactory.CreateFixedPositionMessage(position);
        Logger.LogInformation("Setting fixed position");

        await _deviceConnection.WriteToRadio(_toRadioMessageFactory.CreateMeshPacketMessage(adminMessage), async (_, _) =>
            {
                Logger.LogInformation("Setting fixed position OK");
                await Task.Delay(2000);
                return true;
            });
    }
}