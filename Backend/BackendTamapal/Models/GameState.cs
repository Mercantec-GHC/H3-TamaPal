using BackendTamapal.Models;

namespace BackendTamapal.Models
{
    public class GameState
    {
        public string Status { get; set; } = "waiting";

        // "solo" = én spiller mod væggen, "duo" = opla-1 (venstre) mod opla-2 (højre)
        public string Mode { get; set; } = "solo";

        // Solo: antal slag. Duo: spiller 1's point
        public int Score { get; set; }

        // Duo: spiller 2's point (bruges ikke i solo)
        public int Score2 { get; set; }

        // Solo: liv tilbage (bruges ikke i duo)
        public int Lives { get; set; } = 3;

        // Duo: 0 mens der spilles, 1 eller 2 når der er en vinder
        public int Winner { get; set; }

        public Ball Ball { get; set; } = new Ball();

        // Spiller 1 – venstre ketsjer
        public Paddle Paddle { get; set; } = new Paddle();

        // Spiller 2 – højre ketsjer (kun i duo)
        public Paddle Paddle2 { get; set; } = new Paddle();
    }


}
