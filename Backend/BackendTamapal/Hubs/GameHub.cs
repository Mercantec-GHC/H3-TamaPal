using Microsoft.AspNetCore.SignalR;
using BackendTamapal.Models;

namespace BackendTamapal.Hubs
{
    public class GameHub : Hub
    {
        public async Task StartGame()
        {
            var gameState = new GameState();
            gameState.Status = "playing";
            await Clients.All.SendAsync("GameState", gameState);

        }
    }
}
