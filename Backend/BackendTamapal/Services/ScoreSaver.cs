using BackendTamapal.Data;
using BackendTamapal.Models;
using Microsoft.EntityFrameworkCore;

namespace BackendTamapal.Services
{
    // Gemmer færdige spil i databasen ved siden af spillet,
    // så GameLoop aldrig skal vente på Neon
    public class ScoreSaver : BackgroundService
    {
        private readonly GameService _gameService;
        private readonly IDbContextFactory<TamaPalDbContext> _dbFactory;

        public ScoreSaver(GameService gameService, IDbContextFactory<TamaPalDbContext> dbFactory)
        {
            _gameService = gameService;
            _dbFactory = dbFactory;
        }

        protected override async Task ExecuteAsync(CancellationToken stoppingToken)
        {
            while (!stoppingToken.IsCancellationRequested)
            {
                while (_gameService.FinishedGames.TryDequeue(out GameResult? result))
                {
                    try
                    {
                        // En ny, kortlivet DbContext til hver gemning
                        using var db = _dbFactory.CreateDbContext();
                        db.GameResults.Add(result);
                        await db.SaveChangesAsync(stoppingToken);

                        Console.WriteLine($"Score gemt i databasen: {result.Score} ({result.DeviceId})");
                    }
                    catch (Exception ex)
                    {
                        // Spillet skal virke, selv om databasen er nede
                        Console.WriteLine($"Kunne ikke gemme score: {ex.Message}");
                    }
                }

                await Task.Delay(500, stoppingToken);
            }
        }
    }
}
