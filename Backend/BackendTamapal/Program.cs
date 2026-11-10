using BackendTamapal.Data;
using BackendTamapal.Hubs;
using BackendTamapal.Models;
using BackendTamapal.Services;
using Microsoft.EntityFrameworkCore;
using Microsoft.Extensions.FileProviders;

namespace BackendTamapal
{
    public class Program
    {
        public static void Main(string[] args)
        {
            var builder = WebApplication.CreateBuilder(args);

            // Add services to the container.

            builder.Services.AddControllers();
            builder.Services.AddSignalR();
            builder.Services.AddSingleton<GameService>();
            builder.Services.AddHostedService<GameLoop>();
            builder.Services.AddHostedService<MqttWorker>();

            // Database: connection string kommer fra User Secrets (lokalt) eller ConnectionStrings__TamaPalDb (Dokploy)
            string? connectionString = builder.Configuration.GetConnectionString("TamaPalDb");
            builder.Services.AddDbContextFactory<TamaPalDbContext>(options => options.UseNpgsql(connectionString));
            builder.Services.AddHostedService<ScoreSaver>();

            builder.Services.AddCors(options =>
            {
                options.AddPolicy("Frontend", policy =>
                {
                    policy.WithOrigins("http://localhost:5500","http://127.0.0.1:5500")
                    .AllowAnyHeader()
                    .AllowAnyMethod()
                    .AllowCredentials();

                });
            });

            // Learn more about configuring OpenAPI at https://aka.ms/aspnet/openapi
            builder.Services.AddOpenApi();

            var app = builder.Build();

            // Configure the HTTP request pipeline.
            if (app.Environment.IsDevelopment())
            {
                app.MapOpenApi();
            }

            app.UseHttpsRedirection();

            // Vis Andriis frontend: lokalt fra ../../Frontend, i Docker fra wwwroot
            string frontendFolder = builder.Configuration["FrontendPath"] ?? "wwwroot";
            var frontendFiles = new PhysicalFileProvider(Path.GetFullPath(Path.Combine(builder.Environment.ContentRootPath, frontendFolder)));
            app.UseDefaultFiles(new DefaultFilesOptions { FileProvider = frontendFiles });
            app.UseStaticFiles(new StaticFileOptions
            {
                FileProvider = frontendFiles,
                // "no-cache": Cloudflare og browseren skal spørge serveren hver gang,
                // så en ny version af frontenden ses med det samme efter en deploy
                OnPrepareResponse = context =>
                {
                    context.Context.Response.Headers.CacheControl = "no-cache";
                }
            });

            app.UseCors("Frontend");
            app.UseAuthorization();


            app.MapControllers();
            app.MapHub<GameHub>("/gamehub");

            app.Run();
        }
    }
}
