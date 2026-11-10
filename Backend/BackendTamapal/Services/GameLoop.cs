using Microsoft.AspNetCore.SignalR;
using BackendTamapal.Hubs;
using BackendTamapal.Services;
using System.Diagnostics;

namespace BackendTamapal.Services
{
    public class GameLoop : BackgroundService
    {
        private readonly GameService _gameService;
        private readonly IHubContext<GameHub> _hubContext;

        public GameLoop(GameService gameService, IHubContext<GameHub> hubContext)
        {
            _gameService = gameService;
            _hubContext = hubContext;
        }

        protected override async Task ExecuteAsync(CancellationToken stoppingToken)
        {
            var stopwatch = Stopwatch.StartNew();
            while (!stoppingToken.IsCancellationRequested)
            {
                double dt = Math.Min(stopwatch.Elapsed.TotalSeconds, 0.1);
                stopwatch.Restart();

                _gameService.Tick(dt);
                await Task.Delay(33, stoppingToken);
                await _hubContext.Clients.All.SendAsync("GameState", _gameService.State);

            }
        }

    }
}
