using System.Reactive.Concurrency;
using System.Text.Json;
using Google.Protobuf;
using Meshtastic.Connections;
using Meshtastic.Data;
using Meshtastic.Data.MessageFactories;
using Meshtastic.Protobufs;
using Microsoft.EntityFrameworkCore;
using Monitor.Context;
using Monitor.Context.Entities;
using Monitor.Extensions;
using Monitor.Services;
using Exception = System.Exception;

namespace Monitor.Workers;

public class MeshtasticApp : AEnabledWorker
{
    private readonly SerialMessageService _serialMessageService;
    private readonly DataContext _context;
    private readonly string _path;
    private readonly int _speed;
    private readonly double _latitude, _longitude;
    private readonly int _altitude;
    private readonly bool _telemEnabled, _echoEnabled;
    private readonly uint _channelEcho;
    
    private SerialConnection? _deviceConnection;

    public MeshtasticApp(ILogger<MeshtasticApp> logger, IServiceProvider serviceProvider,
        IConfiguration configuration, IDbContextFactory<DataContext> contextFactory,
        SerialMessageService serialMessageService) : base(logger, serviceProvider)
    {
        _serialMessageService = serialMessageService;
        _context = contextFactory.CreateDbContext();
        
        RetryDuration = TimeSpan.FromSeconds(30);
            
        var configurationSection = configuration.GetSection("Meshtastic");
        _path = configurationSection.GetValueOrThrow<string>("Path");
        _speed = configurationSection.GetValue("Speed", 38400);
        
        _telemEnabled = configurationSection.GetValue("TelemetryEnabled", false);
        
        _echoEnabled = configurationSection.GetValue("EchoEnabled", false);
        _channelEcho = (uint)configurationSection.GetValue("ChannelEcho", 0);
        
        var positionSection = configuration.GetSection("Position");
        _latitude = positionSection.GetValueOrThrow<double>("Latitude");
        _longitude = positionSection.GetValueOrThrow<double>("Longitude");
        _altitude = positionSection.GetValueOrThrow<int>("Altitude");
    }

    protected override async Task Start(CancellationToken cancellationToken)
    {
        _deviceConnection = new SerialConnection(Logger, _path, _speed);
        await _deviceConnection.Start();

        AddDisposable(
            Scheduler.SchedulePeriodic(TimeSpan.FromMinutes(15), SendWeather)
        );
        
        AddDisposable(
            Scheduler.SchedulePeriodic(TimeSpan.FromDays(1), SetPosition)
        );

        _deviceConnection.MessageRecieved += PacketReceived;

        // await SetPosition();
    }

    protected override Task Stop()
    {
        try
        {
            _deviceConnection?.Disconnect();
        }
        catch (Exception e)
        {
            Logger.LogWarning(e, "Impossible to disconnect Meshtastic Serial Port");
        }

        return base.Stop();
    }

    private void PacketReceived(object? sender, MessageRecievedEventArgs e)
    {
        if (_deviceConnection == null)
        {
            throw new Exception("No connection");
        }

        if (e.Message.Packet == null)
        {
            return;
        }
        
        _context.Add(new LoRa
        {
            Frame = JsonSerializer.Serialize(new {
                Payload = e.Message.Packet.Decoded.Payload.ToStringUtf8(),
                Raw = e.Message,
            }),
            Sender = e.DeviceStateContainer.GetNodeDisplayName(e.Message.Packet.From),
            IsTx = false,
            IsMeshtastic = true
        });
        _context.SaveChanges();
        
        DoEcho(e.Message);
    }

    private void DoEcho(FromRadio radio)
    {
        if (!_echoEnabled)
        {
            return;
        }
        
        if (_deviceConnection == null)
        {
            throw new Exception("No connection");
        }
        
        var packet = radio.Packet;
        var packetDecoded = packet.Decoded;

        if (packet.From == _deviceConnection.DeviceStateContainer.MyNodeInfo.MyNodeNum || packet.Channel != _channelEcho || packetDecoded is not { Portnum: PortNum.TextMessageApp })
        {
            return;
        }
        
        var payloadString = packetDecoded.Payload.ToStringUtf8();

        if (!payloadString.StartsWith('?'))
        {
            return;
        }
                
        var textMessageFactory = new TextMessageFactory(_deviceConnection.DeviceStateContainer);
        var textMessagePacket = textMessageFactory.CreateTextMessagePacket($">{payloadString}");
        textMessagePacket.To = packet.From;
        textMessagePacket.Channel = packet.Channel;

        Logger.LogTrace("Sending echo message {message}", payloadString);

        Send(textMessagePacket);
    }

    private void DoCommand(FromRadio radio)
    {
        if (_deviceConnection == null)
        {
            throw new Exception("No connection");
        }
        
        var packet = radio.Packet;
        var packetDecoded = packet.Decoded;

        if (packet.To != _deviceConnection.DeviceStateContainer.MyNodeInfo.MyNodeNum || packetDecoded is not { Portnum: PortNum.TextMessageApp })
        {
            return;
        }
        
        var payloadString = packetDecoded.Payload.ToStringUtf8();

        if (!payloadString.StartsWith('!'))
        {
            return;
        }

        if (_serialMessageService.SendCommand(payloadString.TrimStart('!')))
        {
            payloadString += " OK";
        }
        else
        {
            payloadString += " KO";
        }
        
        var textMessageFactory = new TextMessageFactory(_deviceConnection.DeviceStateContainer);
        var textMessagePacket = textMessageFactory.CreateTextMessagePacket($">{payloadString}");
        textMessagePacket.To = packet.From;
        textMessagePacket.Channel = packet.Channel;
        
        Logger.LogTrace("Sending ack command for {command}", payloadString);

        Send(textMessagePacket);
    }

    private void SendWeather()
    {
        if (!_telemEnabled)
        {
            return;
        }
        
        if (_deviceConnection == null)
        {
            throw new Exception("No connection");
        }
        
        var messageFactory = new TelemetryMessageFactory(_deviceConnection.DeviceStateContainer, 0xffffffff);
        
        var telemetryPacket = messageFactory.CreateTelemetryPacket();
        telemetryPacket.WantAck = false;
        telemetryPacket.Priority = MeshPacket.Types.Priority.Background;
        telemetryPacket.Decoded.WantResponse = false;
        telemetryPacket.Decoded.Payload = new Telemetry
        {
            Time = (uint)DateTime.UtcNow.ToFrench().ToUnixTimestamp(),
            EnvironmentMetrics = new EnvironmentMetrics
            {
                Temperature = EntitiesManagerService.Entities.WeatherTemperature.Value,
                RelativeHumidity = EntitiesManagerService.Entities.WeatherHumidity.Value,
                BarometricPressure = EntitiesManagerService.Entities.WeatherPressure.Value,
                Voltage = EntitiesManagerService.Entities.BatteryVoltage.Value / 1000.0f,
                Current = EntitiesManagerService.Entities.MpptChargeCurrent.Value
            },
            // PowerMetrics = new PowerMetrics
            // {
            //     Ch1Current = 130.56f, // EntitiesManagerService.Entities.BatteryCurrent.Value,
            //     Ch1Voltage = 12.45f, // EntitiesManagerService.Entities.BatteryVoltage.Value / 1000.0f,
            //     Ch2Current = 1256.89f, // EntitiesManagerService.Entities.SolarCurrent.Value,
            //     Ch2Voltage = 13, // EntitiesManagerService.Entities.SolarVoltage.Value / 1000.0f,
            // }
        }.ToByteString();
        
        Logger.LogTrace("Sending telemetry message");
        
        Send(telemetryPacket);
    }

    private void SetPosition()
    {
        if (_deviceConnection == null)
        {
            throw new Exception("No connection");
        }
        
        var adminMessageFactory = new AdminMessageFactory(_deviceConnection.DeviceStateContainer);

        decimal divisor = new(1e-7);
        var position = new Position
        {
            LatitudeI = decimal.ToInt32((decimal)(_latitude / (double)divisor)),
            LongitudeI = decimal.ToInt32((decimal)(_longitude / (double)divisor)),
            Altitude = _altitude
        };

        var adminPaquet = adminMessageFactory.CreateFixedPositionMessage(position);
        Logger.LogInformation("Setting fixed position");

        Send(adminPaquet);
    }

    private void Send(MeshPacket packet)
    {
        if (_deviceConnection == null)
        {
            throw new Exception("No connection");
        }

        var payload = packet.Decoded.Payload.ToStringUtf8();
        Logger.LogInformation("Sending {message} to {to} on channel {channel}", payload, packet.Channel, _deviceConnection.DeviceStateContainer.GetNodeDisplayName(packet.To));
        
        _deviceConnection.Send(_deviceConnection.ToRadioFactory.CreateMeshPacketMessage(packet));
        
        _context.Add(new LoRa
        {
            Frame = JsonSerializer.Serialize(new {
                Payload = payload,
                Raw = packet,
            }),
            Sender = _deviceConnection.DeviceStateContainer.GetNodeDisplayName(_deviceConnection.DeviceStateContainer.MyNodeInfo.MyNodeNum),
            IsTx = true,
            IsMeshtastic = true
        });
        _context.SaveChanges();
    }
}