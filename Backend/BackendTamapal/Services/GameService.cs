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

        public void HandleInput(OplaInput input)
        {
            if (input.Button)
                Start();

            double y = (input.Tilt + 1) / 2;
            State.Paddle.Y = Math.Clamp(y, 0.1, 0.9);
        }

    }

  


}
