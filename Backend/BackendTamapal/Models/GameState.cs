using BackendTamapal.Models;

namespace BackendTamapal.Models
{
    public class GameState
    {
        public string Status { get; set; } = "waiting";
        public int Score { get; set; }
        public int Lives { get; set; } = 3;

        public Ball Ball { get; set; } = new Ball();
        public Paddle Paddle { get; set; } = new Paddle();
    }


}
