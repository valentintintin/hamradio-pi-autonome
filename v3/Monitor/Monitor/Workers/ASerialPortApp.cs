using System.IO.Ports;
using System.Reactive.Concurrency;
using Monitor.Extensions;
using Monitor.Services;

namespace Monitor.Workers;

public abstract class ASerialPortApp : AWorker
{
    protected readonly MonitorService MonitorService;
    private readonly string _fakeInput;
    protected SerialPort? SerialPort;

    private readonly string? _path;
    private readonly int _speed;
    private readonly bool _simulate;

    protected ASerialPortApp(ILogger<ASerialPortApp> logger, IServiceProvider serviceProvider,
        IConfiguration configuration, string configSectionName, string fakeInput = "")
        : base(logger, serviceProvider)
    {
        MonitorService = Services.GetRequiredService<MonitorService>();
        _fakeInput = fakeInput;
        
        var configurationSection = configuration.GetSection(configSectionName);

        _simulate = configurationSection.GetValue("Simulate", false);
        
        if (!_simulate)
        {
            _path = configurationSection.GetValueOrThrow<string>("Path");
            _speed = configurationSection.GetValueOrThrow<int>("Speed");
        }
    }

    protected abstract Task MessageReceived(string input);

    protected override Task Start(CancellationToken cancellationToken)
    {
        if (_simulate)
        {
            SerialMessageService.Simulate = true;
            
            var lines = _fakeInput.Split('\n');
            var currentLine = 0;

            AddDisposable(Scheduler.SchedulePeriodic(TimeSpan.FromMilliseconds(25000), () => 
            {
                var input = lines[currentLine++];
                
                Logger.LogDebug("Received serial message : {input}", input);
                
                MessageReceived(input);
                
                if (currentLine >= lines.Length)
                {
                    currentLine = 0;
                }
            }));
        }
        else
        {
            SerialPort = new SerialPort(_path, _speed);

            // TODO change it for only one app
            SerialMessageService.SerialPort ??= SerialPort;

            SerialPort.NewLine = "\n";
            SerialPort.DataReceived += (_, _) =>
            {
                try
                {
                    var input = SerialPort.ReadLine();
                
                    MessageReceived(input);
                }
                catch (Exception e)
                {
                    Logger.LogError(e, "SerialPort error during read or decode");
                }
            };
            
            try
            {
                SerialPort.Open();
                
                Logger.LogInformation("SerialPort {path}:{speed} opened OK", _path, _speed);
            }
            catch (Exception e)
            {
                Logger.LogCritical(e, "SerialPort {path}:{speed} opened KO", _path, _speed);
            }
        }
        
        return Task.CompletedTask;
    }

    protected override Task Stop()
    {
        SerialPort?.Close();

        return base.Stop();
    }
}