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
        private const double PaddleX = 0.03;

        private const double StartSpeed = 0.5;
        private const double SpeedUp = 1.05;
        private const double MaxSpeed = 1.2;
        private const double PaddleSpeed = 1.0;

        private double _vx;
        private double _vy;
        private int _paddleDirection;
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

            ServeBall();
        }

        public void HandleInput(OplaInput input, string deviceId = "opla-1")
        {
            PlayerDevice = deviceId;

            if (input.Button)
                Start();

            _paddleDirection = (input.Down ? 1 : 0) - (input.Up ? 1 : 0);
        }

        private void SendToOpla(string topicEnd, object payload)
        {
            var message = new OplaMessage
            {
                DeviceId = PlayerDevice,
                TopicEnd = topicEnd,
                Json = JsonSerializer.Serialize(payload)
            };
            Outbox.Enqueue(message);
        }

        private void ServeBall()
        {
            State.Ball.X = 0.5;
            State.Ball.Y = 0.5;

            double angle = (Random.Shared.NextDouble() - 0.5) * (Math.PI / 2);
            _vx = Math.Cos(angle) * StartSpeed;
            _vy = Math.Sin(angle) * StartSpeed;
        }

        public void Tick(double dt)
        {
            if (State.Status != "playing")
                return;

            double newY = State.Paddle.Y + _paddleDirection * PaddleSpeed * dt;
            State.Paddle.Y = Math.Clamp(newY, PaddleHeight / 2, 1 - PaddleHeight / 2);

            Ball ball = State.Ball;
            double previousX = ball.X;

            ball.X += _vx * dt;
            ball.Y += _vy * dt;

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

            if (ball.X + BallRadius > 1)
            {
                ball.X = 1 - BallRadius;
                _vx = -Math.Abs(_vx);
            }

            double paddleRight = PaddleX + PaddleWidth / 2;
            bool crossedPaddle = _vx < 0
                && previousX - BallRadius >= paddleRight
                && ball.X - BallRadius <= paddleRight;
            bool besidePaddle = Math.Abs(ball.Y - State.Paddle.Y) <= PaddleHeight / 2 + BallRadius;

            if (crossedPaddle && besidePaddle)
            {
                ball.X = paddleRight + BallRadius;

                double speed = Math.Sqrt(_vx * _vx + _vy * _vy);
                double factor = Math.Min(SpeedUp, MaxSpeed / speed);
                _vx = Math.Abs(_vx) * factor;
                _vy = _vy * factor;

                State.Score++;
                SendToOpla("event", new { type = "hit" });
            }

            if (ball.X + BallRadius < 0)
            {
                State.Lives--;

                if (State.Lives <= 0)
                {
                    State.Status = "gameover";
                    SendToOpla("event", new { type = "gameover" });

                    FinishedGames.Enqueue(new GameResult { Score = State.Score, DeviceId = PlayerDevice });

                    int reward = Math.Min(State.Score * 3, 30);
                    if (reward > 0)
                        SendToOpla("pet/reward", new { type = "happy_boost", amount = reward });
                }
                else
                {
                    SendToOpla("event", new { type = "miss" });
                    ServeBall();
                }
            }
        }
    }

  


}
