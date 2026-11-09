using System.Collections.Concurrent;
using System.Text.Json;
using BackendTamapal.Models;

namespace BackendTamapal.Services
{
    public class GameService
    {

        private const double BallRadius = 0.02;
        private const double PaddleWidth = 0.02;
        private const double PaddleHeight = 0.2;
        private const double PaddleX = 0.03;   // spiller 1 – venstre ketsjer
        private const double Paddle2X = 0.97;  // spiller 2 – højre ketsjer (kun duo)

        private const double StartSpeed = 0.5;
        private const double SpeedUp = 1.05;
        private const double MaxSpeed = 1.2;
        private const double PaddleSpeed = 1.0;

        // Duo (2 spillere)
        private const string Player1Device = "opla-1";
        private const string Player2Device = "opla-2";
        private const int WinningScore = 5;      // først til 5 point vinder
        private const int WinnerReward = 25;     // happy_boost til vinderen
        private const double ActiveSeconds = 3;  // en Oplà er "med", hvis den har sendt input inden for 3 sek

        private double _vx;
        private double _vy;
        private int _paddleDirection1;
        private int _paddleDirection2;
        private DateTime _lastSeen1 = DateTime.MinValue;
        private DateTime _lastSeen2 = DateTime.MinValue;
        public GameState State { get; set; } = new GameState();

        public ConcurrentQueue<OplaMessage> Outbox { get; } = new();

        // Færdige spil venter her, til ScoreSaver gemmer dem i databasen
        public ConcurrentQueue<GameResult> FinishedGames { get; } = new();
        public string PlayerDevice { get; private set; } = "opla-1";



        public void Start()
        {
            if (State.Status == "playing")
                return;

            State = new GameState();

            State.Status = "playing";
            State.Mode = BothPlayersActive() ? "duo" : "solo";

            ServeBall(1);
        }

        public void HandleInput(OplaInput input, string deviceId = "opla-1")
        {
            PlayerDevice = deviceId;

            int direction = (input.Down ? 1 : 0) - (input.Up ? 1 : 0);

            // Husk retning og tidspunkt for hver Oplà for sig
            if (deviceId == Player2Device)
            {
                _paddleDirection2 = direction;
                _lastSeen2 = DateTime.UtcNow;
            }
            else
            {
                _paddleDirection1 = direction;
                _lastSeen1 = DateTime.UtcNow;
            }

            if (input.Button)
                Start();
        }

        // Duo, hvis begge Oplàer har sendt input inden for de sidste 3 sekunder
        private bool BothPlayersActive()
        {
            DateTime now = DateTime.UtcNow;
            bool player1Active = (now - _lastSeen1).TotalSeconds < ActiveSeconds;
            bool player2Active = (now - _lastSeen2).TotalSeconds < ActiveSeconds;
            return player1Active && player2Active;
        }

        private void SendToOpla(string deviceId, string topicEnd, object payload)
        {
            var message = new OplaMessage
            {
                DeviceId = deviceId,
                TopicEnd = topicEnd,
                Json = JsonSerializer.Serialize(payload)
            };
            Outbox.Enqueue(message);
        }

        // direction: 1 = bolden starter mod højre, -1 = mod venstre
        private void ServeBall(int direction)
        {
            State.Ball.X = 0.5;
            State.Ball.Y = 0.5;

            // Vinkel mellem 15° og 45° op eller ned. Aldrig helt vandret, for så kan ketsjerne
            // bare stå stille, og spillet går i stå
            double angle = (15 + Random.Shared.NextDouble() * 30) * Math.PI / 180;
            if (Random.Shared.Next(2) == 0)
                angle = -angle;

            _vx = direction * Math.Cos(angle) * StartSpeed;
            _vy = Math.Sin(angle) * StartSpeed;
        }

        private void MovePaddle(Paddle paddle, int direction, double dt)
        {
            double newY = paddle.Y + direction * PaddleSpeed * dt;
            paddle.Y = Math.Clamp(newY, PaddleHeight / 2, 1 - PaddleHeight / 2);
        }

        // Er bolden ud for ketsjeren (lodret)?
        private bool BesidePaddle(Paddle paddle)
        {
            return Math.Abs(State.Ball.Y - paddle.Y) <= PaddleHeight / 2 + BallRadius;
        }

        // Send bolden tilbage og gør den lidt hurtigere. direction: 1 = mod højre, -1 = mod venstre
        private void Bounce(int direction)
        {
            double speed = Math.Sqrt(_vx * _vx + _vy * _vy);
            double factor = Math.Min(SpeedUp, MaxSpeed / speed);
            _vx = direction * Math.Abs(_vx) * factor;
            _vy = _vy * factor;
        }

        public void Tick(double dt)
        {
            if (State.Status != "playing")
                return;

            bool duo = State.Mode == "duo";

            // 1. Flyt ketsjerne
            if (duo)
            {
                MovePaddle(State.Paddle, _paddleDirection1, dt);
                MovePaddle(State.Paddle2, _paddleDirection2, dt);
            }
            else
            {
                int soloDirection = PlayerDevice == Player2Device ? _paddleDirection2 : _paddleDirection1;
                MovePaddle(State.Paddle, soloDirection, dt);
            }

            // 2. Flyt bolden
            Ball ball = State.Ball;
            double previousX = ball.X;

            ball.X += _vx * dt;
            ball.Y += _vy * dt;

            // 3. Top og bund
            if (ball.Y - BallRadius < 0)
            {
                ball.Y = BallRadius;
                _vy = Math.Abs(_vy);
            }
            if (ball.Y + BallRadius > 1)
            {
                ball.Y = 1 - BallRadius;
                _vy = -Math.Abs(_vy);
            }

            // 4. Højre væg – kun i solo (i duo står spiller 2 der)
            if (!duo && ball.X + BallRadius > 1)
            {
                ball.X = 1 - BallRadius;
                _vx = -Math.Abs(_vx);
            }

            // 5. Venstre ketsjer (spiller 1)
            double leftFace = PaddleX + PaddleWidth / 2;
            bool crossedLeft = _vx < 0
                && previousX - BallRadius >= leftFace
                && ball.X - BallRadius <= leftFace;

            if (crossedLeft && BesidePaddle(State.Paddle))
            {
                ball.X = leftFace + BallRadius;
                Bounce(1);

                if (duo)
                {
                    SendToOpla(Player1Device, "event", new { type = "hit" });
                }
                else
                {
                    State.Score++;
                    SendToOpla(PlayerDevice, "event", new { type = "hit" });
                }
            }

            // 6. Højre ketsjer (spiller 2) – kun i duo
            double rightFace = Paddle2X - PaddleWidth / 2;
            bool crossedRight = duo
                && _vx > 0
                && previousX + BallRadius <= rightFace
                && ball.X + BallRadius >= rightFace;

            if (crossedRight && BesidePaddle(State.Paddle2))
            {
                ball.X = rightFace - BallRadius;
                Bounce(-1);
                SendToOpla(Player2Device, "event", new { type = "hit" });
            }

            // 7. Bolden er ude
            if (ball.X + BallRadius < 0)
            {
                if (duo)
                    ScorePoint(2);   // spiller 1 missede, så spiller 2 får pointet
                else
                    LoseLife();
            }
            else if (duo && ball.X - BallRadius > 1)
            {
                ScorePoint(1);       // spiller 2 missede, så spiller 1 får pointet
            }
        }

        // Solo: et liv mindre – ved 0 liv er spillet slut
        private void LoseLife()
        {
            State.Lives--;

            if (State.Lives <= 0)
            {
                State.Status = "gameover";
                SendToOpla(PlayerDevice, "event", new { type = "gameover" });

                FinishedGames.Enqueue(new GameResult { Score = State.Score, DeviceId = PlayerDevice, Mode = "solo" });

                int reward = Math.Min(State.Score * 3, 30);
                if (reward > 0)
                    SendToOpla(PlayerDevice, "pet/reward", new { type = "happy_boost", amount = reward });
            }
            else
            {
                SendToOpla(PlayerDevice, "event", new { type = "miss" });
                ServeBall(1);
            }
        }

        // Duo: spilleren, der IKKE missede, får et point – først til 5 vinder
        private void ScorePoint(int player)
        {
            if (player == 1)
                State.Score++;
            else
                State.Score2++;

            string scorerDevice = player == 1 ? Player1Device : Player2Device;
            string missedDevice = player == 1 ? Player2Device : Player1Device;
            int points = player == 1 ? State.Score : State.Score2;

            if (points >= WinningScore)
            {
                State.Status = "gameover";
                State.Winner = player;

                SendToOpla(Player1Device, "event", new { type = "gameover" });
                SendToOpla(Player2Device, "event", new { type = "gameover" });
                SendToOpla(scorerDevice, "pet/reward", new { type = "happy_boost", amount = WinnerReward });

                FinishedGames.Enqueue(new GameResult { Score = State.Score, DeviceId = Player1Device, Mode = "duo" });
                FinishedGames.Enqueue(new GameResult { Score = State.Score2, DeviceId = Player2Device, Mode = "duo" });
            }
            else
            {
                SendToOpla(missedDevice, "event", new { type = "miss" });

                // Næste bold sendes mod den, der missede: spiller 2 står til højre (1), spiller 1 til venstre (-1)
                ServeBall(player == 1 ? 1 : -1);
            }
        }
    }




}
