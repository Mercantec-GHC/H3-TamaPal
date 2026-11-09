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

        private double _vx;
        private double _vy;
        public GameState State { get; set; } = new GameState();



        public void Start()
        {
            if (State.Status == "playing")
                return;

            State = new GameState();

            State.Status = "playing";

            ServeBall();
        }

        public void HandleInput(OplaInput input)
        {
            if (input.Button)
                Start();

            double y = (input.Tilt + 1) / 2;
            State.Paddle.Y = Math.Clamp(y, 0.1, 0.9);
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
            }

            if (ball.X + BallRadius < 0)
            {
                State.Lives--;

                if (State.Lives <= 0)
                    State.Status = "gameover";
                else
                    ServeBall();
            }
        }
    }

  


}
