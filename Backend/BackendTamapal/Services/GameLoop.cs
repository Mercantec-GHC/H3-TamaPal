using Microsoft.AspNetCore.SignalR;
using BackendTamapal.Hubs;
using BackendTamapal.Services;

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
            while (!stoppingToken.IsCancellationRequested)
            {
                await Task.Delay(33, stoppingToken);
                await _hubContext.Clients.All.SendAsync("GameState", _gameService.State);

            }
        }

    }
}
