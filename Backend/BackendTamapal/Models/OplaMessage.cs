namespace BackendTamapal.Models
{
    public class OplaMessage
    {
        public string DeviceId { get; set; } = "opla-1";
        public string TopicEnd { get; set; } = "event";
        public string Json { get; set; } = "";
    }
}