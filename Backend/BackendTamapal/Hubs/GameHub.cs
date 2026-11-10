using Microsoft.AspNetCore.SignalR;
using BackendTamapal.Models;
using BackendTamapal.Services;


namespace BackendTamapal.Hubs
{
    public class GameHub : Hub
    {

        private readonly GameService _gameService;

        public GameHub(GameService gameService)
        {
            _gameService = gameService;
        }

        public async Task StartGame()
        {

            _gameService.Start();
            await Clients.All.SendAsync("GameState", _gameService.State);

        }



    }


}
