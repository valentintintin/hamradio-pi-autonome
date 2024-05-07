using System.Reactive.Concurrency;
using Google.Protobuf;
using Meshtastic.Connections;
using Meshtastic.Data.MessageFactories;
using Meshtastic.Protobufs;
using Monitor.Extensions;
using Monitor.Models;
using Monitor.Services;
using Exception = System.Exception;

namespace Monitor.Workers;

public class MeshtasticApp : AEnabledWorker
{
    private readonly SerialMessageService _serialMessageService;
    private readonly MonitorService _monitorService;
    private readonly string _path;
    private readonly int _speed;
    private readonly double _latitude, _longitude;
    private readonly int _altitude;
    private readonly bool _telemEnabled, _echoEnabled;
    private readonly uint _channelEcho;
    private readonly int _currentYear;
    private readonly TimeSpan _intervalTelemetry;
    
    private SerialConnection? _deviceConnection;

    public MeshtasticApp(ILogger<MeshtasticApp> logger, IServiceProvider serviceProvider,
        IConfiguration configuration, SerialMessageService serialMessageService, MonitorService monitorService) : base(logger, serviceProvider)
    {
        _serialMessageService = serialMessageService;
        _monitorService = monitorService;
        
        RetryDuration = TimeSpan.FromSeconds(30);

        _currentYear = configuration.GetValueOrThrow<int>("CurrentYear");
        
        var configurationSection = configuration.GetSection("Meshtastic");
        _path = configurationSection.GetValueOrThrow<string>("Path");
        _speed = configurationSection.GetValue("Speed", 38400);
        
        _telemEnabled = configurationSection.GetValue("TelemetryEnabled", false);
        _intervalTelemetry = TimeSpan.FromMinutes(configurationSection.GetValue("PositionAndTelemetryInterval", 15));
        
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
            Scheduler.SchedulePeriodic(_intervalTelemetry, SendWeather)
        );
        
        AddDisposable(
            Scheduler.SchedulePeriodic(_intervalTelemetry, SetPosition)
        );

        _deviceConnection.MessageRecieved += PacketReceived;
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

        var nodes = e.DeviceStateContainer.Nodes
            .Skip(1) // it's us
            .Select(n => new MeshtasticNode(n))
            .ToList();
        
        _monitorService.UpdateMeshtasticNodesOnlines(nodes);

        if (DateTime.UtcNow.Year == _currentYear && nodes.Any(n => n.LastHeardFromNow == TimeSpan.Zero))
        {
            SetPosition();
        }

        var packet = e.Message.Packet;
        if (packet == null)
        {
            return;
        }
        
        var payload = packet.Decoded.Portnum.ToString();
        if (packet.Decoded.Portnum == PortNum.TextMessageApp)
        {
            payload = packet.Decoded.Payload.ToStringUtf8();
        }
        Logger.LogInformation("Receiving {message} on channel {channel} from {from} to {to}", payload, packet.Channel, _deviceConnection.DeviceStateContainer.GetNodeDisplayName(packet.From), _deviceConnection.DeviceStateContainer.GetNodeDisplayName(packet.To));
        
        _monitorService.AddLoRaMeshtasticMessage(packet, e.DeviceStateContainer.GetNodeDisplayName(packet.From), false);

        if (!DoEcho(e.Message))
        {
            DoCommand(e.Message);
        }
    }

    private bool DoEcho(FromRadio radio)
    {
        if (!_echoEnabled)
        {
            return false;
        }
        
        if (_deviceConnection == null)
        {
            throw new Exception("No connection");
        }
        
        var packet = radio.Packet;
        var packetDecoded = packet.Decoded;

        if (packet.From == _deviceConnection.DeviceStateContainer.MyNodeInfo.MyNodeNum || packet.Channel != _channelEcho || packetDecoded is not { Portnum: PortNum.TextMessageApp })
        {
            return false;
        }
        
        var payloadString = packetDecoded.Payload.ToStringUtf8();

        if (!payloadString.StartsWith('?'))
        {
            return false;
        }
        
        var textMessageFactory = new TextMessageFactory(_deviceConnection.DeviceStateContainer);
        var textMessagePacket = textMessageFactory.CreateTextMessagePacket($">{payloadString}");
        textMessagePacket.To = packet.From;
        textMessagePacket.Channel = packet.Channel;

        Logger.LogTrace("Sending echo message {message}", payloadString);

        Send(textMessagePacket);

        return true;
    }

    private bool DoCommand(FromRadio radio)
    {
        if (_deviceConnection == null)
        {
            throw new Exception("No connection");
        }
        
        var packet = radio.Packet;
        var packetDecoded = packet.Decoded;

        if (packet.To != _deviceConnection.DeviceStateContainer.MyNodeInfo.MyNodeNum || packetDecoded is not { Portnum: PortNum.TextMessageApp })
        {
            return false;
        }
        
        var payloadString = packetDecoded.Payload.ToStringUtf8();

        if (!payloadString.StartsWith('!'))
        {
            return false;
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

        return true;
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
        
        var positionMessageFactory = new PositionMessageFactory(_deviceConnection.DeviceStateContainer);

        decimal divisor = new(1e-7);
        var position = new Position
        {
            LatitudeI = decimal.ToInt32((decimal)(_latitude / (double)divisor)),
            LongitudeI = decimal.ToInt32((decimal)(_longitude / (double)divisor)),
            Altitude = _altitude,
            Time = (uint)DateTime.UtcNow.ToFrench().ToUnixTimestamp()
        };

        var positionPacket = positionMessageFactory.CreatePositionPacket(position);
        positionPacket.WantAck = false;
        positionPacket.Priority = MeshPacket.Types.Priority.Background;
        
        Logger.LogInformation("Setting position and time");

        Send(positionPacket);
    }

    private void Send(MeshPacket packet)
    {
        if (_deviceConnection == null)
        {
            throw new Exception("No connection");
        }

        var payload = packet.Decoded.Portnum.ToString();
        if (packet.Decoded.Portnum == PortNum.TextMessageApp)
        {
            payload = packet.Decoded.Payload.ToStringUtf8();
        }
        Logger.LogInformation("Sending {message} on channel {channel} to {to}", payload, packet.Channel, _deviceConnection.DeviceStateContainer.GetNodeDisplayName(packet.To));
        
        _deviceConnection.Send(_deviceConnection.ToRadioFactory.CreateMeshPacketMessage(packet));
        
        _monitorService.AddLoRaMeshtasticMessage(packet, _deviceConnection.DeviceStateContainer.GetNodeDisplayName(_deviceConnection.DeviceStateContainer.MyNodeInfo.MyNodeNum), true);
    }
}