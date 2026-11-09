namespace BackendTamapal.Models
{
    // Ét færdigt spil = én række i tabellen GameResults i Neon
    public class GameResult
    {
        public int Id { get; set; }
        public int Score { get; set; }
        public string DeviceId { get; set; } = "opla-1";

        // Npgsql kræver UTC-tid, så vi sætter den automatisk, når objektet bliver lavet
        public DateTime PlayedAt { get; set; } = DateTime.UtcNow;
    }
}
