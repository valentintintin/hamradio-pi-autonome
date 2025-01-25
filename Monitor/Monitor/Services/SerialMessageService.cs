using System.IO.Ports;
using System.Text.Json;
using Monitor.Exceptions;
using Monitor.Models.SerialMessages;

namespace Monitor.Services;

public class SerialMessageService(ILogger<SerialMessageService> logger) : AService(logger)
{
    public static SerialPort? SerialPort { get; set; }
    public static bool Simulate { get; set; }

    public Message ParseMessage(string input)
    {
        Logger.LogTrace("Received serial message : {input}", input);

        Message? message, messageTyped;
        
        try
        {
            message = JsonSerializer.Deserialize<Message>(input);
        }
        catch (Exception e)
        {
            if (input.Contains("lora")) // because APRS can have char " in string and we do not escape it in C++
            {
                const string payloadString = "payload\":\"";
                
                message = new LoraData
                {
                    Type = "lora",
                    State = input.Contains("tx") ? "tx" : "rx",
                    Payload = input[(input.IndexOf(payloadString, StringComparison.Ordinal) + payloadString.Length)..input.LastIndexOf('"')]
                };

                input = JsonSerializer.Serialize(message, typeof(LoraData));
            }
            else
            {
                Logger.LogError(e, "Received serial message KO. Deserialize impossible : {input}", input);

                throw new MessageParseException(e, input);
            }
        }

        if (message == null)
        {
            Logger.LogError("Received serial message KO. Message null : {input}", input);

            throw new MessageParseException(input);
        }

        try
        {
            messageTyped = message.Type switch
            {
                "system" => JsonSerializer.Deserialize<McuSystemData>(input),
                "weather" => JsonSerializer.Deserialize<WeatherData>(input),
                "mppt" => JsonSerializer.Deserialize<MpptData>(input),
                "lora" => JsonSerializer.Deserialize<LoraData>(input),
                "gpio" => JsonSerializer.Deserialize<GpioData>(input),
                _ => null
            };
        }
        catch (Exception e)
        {
            Logger.LogError(e, "Received serial message KO. Sub Deserialize impossible : {input}", input);

            throw new MessageParseException(e, input);
        }
        
        return messageTyped ?? message;
    }

    public void SetPowerOnOffVoltage(int powerOnVoltage, int powerOffVoltage)
    {
        SendCommand($"pow {powerOnVoltage} {powerOffVoltage}");
    }

    public void SetWatchdog(TimeSpan timeOff)
    {
        SendCommand($"dog {timeOff.TotalSeconds}");
    }

    public void SetWifi(bool enabled)
    {
        SendCommand($"relay1 {(enabled ? "1" : "0")}");
    }

    public void SetNpr(bool enabled)
    {
        SendCommand($"relay2 {(enabled ? "1" : "0")}");
    }

    public void SetMeshtastic(bool enabled)
    {
        SetWifi(enabled);
    }

    public void SendTelemetry()
    {
        SendCommand("telem");
    }

    public void SendTelemetryParams()
    {
        SendCommand("telemParam");
    }

    public void ResetMcu()
    {
        SendCommand("reset");
    }

    public void Sleep(TimeSpan timeOff)
    {
        SendCommand($"sleep {timeOff.TotalMilliseconds}");
    }

    public void SetTime(DateTime dateTime)
    {
        SendCommand($"time {new DateTimeOffset(dateTime.ToUniversalTime()).ToUnixTimeSeconds()}");
    }

    public void SetWatchdogSafety(bool enabled)
    {
        SetEepromMcu(0, enabled ? 1 : 0);
    }

    public void SetAprsDigipeater(bool enabled)
    {
        SetEepromMcu(1, enabled ? 1 : 0);
    }

    public void SetAprsTelemetry(bool enabled)
    {
        SetEepromMcu(2, enabled ? 1 : 0);
    }

    public void SetAprsPosition(bool enabled)
    {
        SetEepromMcu(3, enabled ? 1 : 0);
    }

    public void SetSleep(bool enabled)
    {
        SetEepromMcu(4, enabled ? 1 : 0);
    }

    public void SetResetOnError(bool enabled)
    {
        SetEepromMcu(5, enabled ? 1 : 0);
    }

    public void SendLora(string message)
    {
        if (string.IsNullOrWhiteSpace(message))
        {
            return;
        }
        
        SendCommand($"lora \"{message}\"");
    }

    public void GetDataJson()
    {
        SendCommand($"jsons");
    }

    private void SetEepromMcu(int address, int value)
    {
        SendCommand($"set {address} {value}");
    }

    public bool SendCommand(string command)
    {
        Logger.LogInformation("Send Serial Command {command}", command);

        if (Simulate)
        {
            return true;
        }
        
        if (SerialPort == null)
        {
            Logger.LogError("Send Serial Command impossible {command}", command);
            return false;
        }
        
        try
        {
            SerialPort.WriteLine(command);
            return true;
        }
        catch (Exception e)
        {
            Logger.LogError(e, "Send Serial Command in error {command}", command);
            return false;
        }
    }
}