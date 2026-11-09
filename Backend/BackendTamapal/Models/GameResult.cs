namespace BackendTamapal.Models
{
    // Ét færdigt spil = én række i tabellen GameResults i Neon (i duo: én række pr. spiller)
    public class GameResult
    {
        public int Id { get; set; }
        public int Score { get; set; }
        public string DeviceId { get; set; } = "opla-1";

        // "solo" (score = antal slag) eller "duo" (score = point, højst 5)
        public string Mode { get; set; } = "solo";

        // Npgsql kræver UTC-tid, så vi sætter den automatisk, når objektet bliver lavet
        public DateTime PlayedAt { get; set; } = DateTime.UtcNow;
    }
}
