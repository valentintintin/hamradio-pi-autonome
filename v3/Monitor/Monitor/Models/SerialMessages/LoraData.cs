using System.Reactive.Linq;
using System.Text.Json.Serialization;

namespace Monitor.Models.SerialMessages;

public class LoraData : Message
{
    [JsonPropertyName("state")]
    public required string State { get; set; }

    [JsonPropertyName("payload")]
    public required string Payload { get; set; }

    [JsonIgnore]
    public bool IsTx => State == "tx";
}