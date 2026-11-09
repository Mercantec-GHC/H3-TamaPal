using BackendTamapal.Models;
using Microsoft.EntityFrameworkCore;

namespace BackendTamapal.Data
{
    // Broen mellem C# og databasen i Neon
    public class TamaPalDbContext : DbContext
    {
        // Indstillingerne (fx connection string) kommer fra Program.cs
        public TamaPalDbContext(DbContextOptions<TamaPalDbContext> options) : base(options)
        {
        }

        // Tabellen GameResults: én række pr. færdigt spil
        public DbSet<GameResult> GameResults => Set<GameResult>();
    }
}
