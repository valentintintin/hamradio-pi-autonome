using System.ComponentModel.DataAnnotations;

namespace Monitor.Context.Entities;

public class Weather : IEntity
{
    [Key]
    public long Id { get; set; }
    public DateTime CreatedAt { get; set; }
    
    public float Temperature { get; set; }
    
    public float Humidity { get; set; } 
    
    public float Pressure { get; set; } 
}