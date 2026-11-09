using MQTTnet;
using MQTTnet.Formatter;
using MQTTnet.Protocol;
using System.Text.Json;
using BackendTamapal.Models;

namespace BackendTamapal.Services
{
    public class MqttWorker : BackgroundService
    {

        private readonly IConfiguration _configuration;
        private readonly GameService _gameService;
        private readonly JsonSerializerOptions _jsonOptions = new() { PropertyNameCaseInsensitive = true };

        public MqttWorker(IConfiguration configuration, GameService gameService)
        {
            _configuration = configuration;
            _gameService = gameService;
        }
        protected override async Task ExecuteAsync(CancellationToken stoppingToken)
        {
            var factory = new MqttClientFactory();
            var client = factory.CreateMqttClient();

            string host = _configuration["Mqtt:Host"] ?? "localhost";
            int port = _configuration.GetValue<int>("Mqtt:Port");
            var options = new MqttClientOptionsBuilder().WithTcpServer(host, port).WithClientId("tamapal-backend").WithProtocolVersion(MqttProtocolVersion.V311).Build();

            client.ApplicationMessageReceivedAsync += e =>
            {
                try
                {
                    string topic = e.ApplicationMessage.Topic;
                    string deviceId = topic.Split('/')[2];

                    string payload = e.ApplicationMessage.ConvertPayloadToString();
                    OplaInput? input = JsonSerializer.Deserialize<OplaInput>(payload, _jsonOptions);

                    if (input != null)
                        _gameService.HandleInput(input, deviceId);
                }
                catch (JsonException)
                {
                    Console.WriteLine("Ugyldig besked fra Oplà blev ignoreret");
                }

                return Task.CompletedTask;
            };

            try
            {
                await client.ConnectAsync(options, stoppingToken);

                var sub = factory.CreateSubscribeOptionsBuilder().WithTopicFilter("StormLamaerne/Tamapal/+/input").Build();
                await client.SubscribeAsync(sub, stoppingToken);
                Console.WriteLine("MQTT forbundet og lytter på input");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"MQTT virker ikke: {ex.Message}");
                return;
            }

            while (!stoppingToken.IsCancellationRequested)
            {
                while (_gameService.Outbox.TryDequeue(out OplaMessage? message))
                {
                    try
                    {
                        var mqttMessage = new MqttApplicationMessageBuilder()
                            .WithTopic($"StormLamaerne/Tamapal/{message.DeviceId}/{message.TopicEnd}")
                            .WithPayload(message.Json)
                            .WithQualityOfServiceLevel(MqttQualityOfServiceLevel.AtLeastOnce)
                            .Build();

                        await client.PublishAsync(mqttMessage, stoppingToken);
                        Console.WriteLine($"Sendt til {message.DeviceId}/{message.TopicEnd}: {message.Json}");
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine($"Kunne ikke sende til Oplà: {ex.Message}");
                    }
                }

                await Task.Delay(20, stoppingToken);
            }


        }

        
    }
}
