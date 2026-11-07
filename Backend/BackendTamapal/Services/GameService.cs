using BackendTamapal.Models;

namespace BackendTamapal.Services
{
    public class GameService
    {
        public GameState State { get; set; } = new GameState();



        public void Start()
        {
            if (State.Status == "playing")
                return;

            State = new GameState();

            State.Status = "playing";


        }

    }

  


}
