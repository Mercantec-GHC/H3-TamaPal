using BackendTamapal.Data;
using BackendTamapal.Models;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;

namespace BackendTamapal.Controllers
{
    // Browseren (eller en lærer) kan hente scorerne her – aldrig direkte fra databasen
    [ApiController]
    [Route("api/scores")]
    public class ScoresController : ControllerBase
    {
        private readonly IDbContextFactory<TamaPalDbContext> _dbFactory;

        public ScoresController(IDbContextFactory<TamaPalDbContext> dbFactory)
        {
            _dbFactory = dbFactory;
        }

        // GET /api/scores – de 10 højeste scorer (highscore)
        [HttpGet]
        public async Task<List<GameResult>> GetHighscores()
        {
            using var db = _dbFactory.CreateDbContext();
            return await db.GameResults
                .OrderByDescending(r => r.Score)
                .Take(10)
                .ToListAsync();
        }

        // GET /api/scores/latest – de 10 seneste spil
        [HttpGet("latest")]
        public async Task<List<GameResult>> GetLatest()
        {
            using var db = _dbFactory.CreateDbContext();
            return await db.GameResults
                .OrderByDescending(r => r.PlayedAt)
                .Take(10)
                .ToListAsync();
        }
    }
}
